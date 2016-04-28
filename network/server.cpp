#include "cache.h"
#include "user_con.hpp"
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <unistd.h>
#include <thread>

using stdthread = std::thread;//resolves namespace conflict
using namespace std;

constexpr int64_t THREADS_PER_VCORE = 2;

class ExitException : public exception {
    virtual const char* what() const throw()
    {
        return "Exited cleanly";
    }
};
void run_server(int tcp_port, int num_tcp_ports, int udp_port, int num_udp_ports, int maxmem);
/*uint64_t custom_hash(key_type *str){
    //taken from internet
    uint64_t hash = 5381;
    uint8_t * iter_str = (uint8_t*)(str);
    uint8_t c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; // hash * 33 + c

    return hash;
}*/
int main(int argc, char** argv)
{
    //taken mainly off the get_opt wikipedia page
    int c;
    int tcp_portnum = 10700;
    int udp_portnum = 10900;
    int num_tcps = 200;
    int num_udps = 200;
    int maxmem = 1 << 16;
    while ((c = getopt(argc, argv, "m:p:u:t:s:")) != -1) {
        switch (c) {
        case 'm':
            maxmem = atoi(optarg);
            break;
        case 'p':
            tcp_portnum = atoi(optarg);
            break;
        case 'u':
            udp_portnum = atoi(optarg);
            break;
        case 't':
            num_tcps = atoi(optarg);
            break;
        case 's':
            num_udps = atoi(optarg);
            break;
        default:
            cout << "bad command line argument";
            return 1;
        }
    }
    run_server(tcp_portnum, num_tcps, udp_portnum, num_udps, maxmem);


    return 0;
}
size_t make_json(bufarr & buf,char * key, char * value)
{
    string begstr = "{ \"key\": \"";
    string midstr = "\" , \"value\": \"";
    string endstr = "\" } ";

    size_t keysize = strlen(key);
    size_t valsize = strlen(value);

    size_t end_size = begstr.size() + keysize + midstr.size() + valsize + endstr.size() + 2;
    if(end_size > bufsize){
        throw runtime_error("value and key too big to jsonify");
    }

    auto ptr = buf.begin();
    ptr = copy(begstr.begin(),begstr.end(),ptr);
    ptr = copy(key,key+keysize,ptr);
    ptr = copy(midstr.begin(),midstr.end(),ptr);
    ptr = copy(value,value+valsize,ptr);
    ptr = copy(endstr.begin(),endstr.end(),ptr);
    *ptr = 0;ptr++;
    *ptr = 0;ptr++;
    //printf("%s\n\n\n",buf.data());
    return end_size;
}
class safe_cache {
public:
    cache_t impl = nullptr;
    safe_cache(cache_t newc)
    {
        impl = newc;
    }
    safe_cache(safe_cache&) = delete;
    ~safe_cache()
    {
        if (impl != nullptr) {
            destroy_cache(impl);
            impl = nullptr;
        }
    }
    cache_t get()
    {
        return impl;
    }
};
template <typename con_ty>
void get(con_ty& con, char * key)
{
    uint32_t val_size = 0;
    val_type v = cache_get(con.cache(), (key_type)(key), &val_size);

    if (v != nullptr) {
        bufarr buf;
        size_t jsonsize = make_json(buf,key,(char*)(v));
        con.write_message(buf.data(),jsonsize);
    } else {
        con.return_error();
    }
}
template <typename con_ty>
void put(con_ty& con, char * keypart, char * valuepart,size_t valarrsize)
{
    cache_set(con.cache(), (key_type)(keypart), (val_type)(valuepart), valarrsize);
}
template <typename con_ty>
void delete_(con_ty& con, char * key)
{
    cache_delete(con.cache(), (key_type)(key));
}
template <typename con_ty>
void head(con_ty&)
{
    //todo:implement!
}
template <typename con_ty>
void post(con_ty& con, char * post_type, char * extrainfo)
{
    //speed does not matter here
    string post_ty(post_type);
    string extinfo(extrainfo);

    strip(post_ty);
    if (post_ty == "shutdown") {
        throw ExitException();
    } else if (post_ty == "memsize") {
        if (cache_space_used(con.cache()) == 0) {
            destroy_cache(con.cache());
            con.cache() = create_cache(stoll(extinfo), NULL);
        } else {
            con.return_error();
        }
    } else {
        throw runtime_error("bad POST message");
    }
}
size_t find(char * buf,char c,size_t start,size_t end){
    for(size_t i = start; i < end; i++){
        if(buf[i] == c){
            return i;
        }
    }
    return string::npos;
}

template <typename con_ty>
void act_on_message(con_ty& con, char * message,size_t messize)
    //messize is size of char array, not of string
{
    size_t first_space = find(message,' ',0,messize);
    size_t first_slash = find(message,'/',first_space + 1,messize);
    size_t second_slash = find(message,'/',first_slash + 1,messize);

    message[first_space] = 0;//null termination of head
    string fword(message);//should't allocate due to small string optimization

    char * info1 = message + first_slash + 1;

    char * info2 = message + second_slash + 1;//null terminated at end

    bool second_exists = second_slash != string::npos;

    size_t end_first = second_exists ? second_slash+1 : messize;
    message[end_first-1] = 0;//null termination of first string

    //makes sure there is no garbage on end of words
    //message[whitespace_begin(message,end_first)] = 0;
    //message[whitespace_begin(message,messize)] = 0;

    if (fword == "GET") {
        get(con, info1);
    } else if (fword == "PUT") {
        put(con,info1,info2,messize-end_first);
    } else if (fword == "DELETE") {
        delete_(con, info1);
    } else if (fword == "HEAD") {
        head(con);
    } else if (fword == "POST") {
        post(con, info1, info2);
    } else {
        throw runtime_error("bad message");
    }
}
template<typename con_ty>
void process_message(con_ty & con,char * recbuf,size_t bytes_written){
    if (bytes_written < bufsize-1) {
        size_t endarr = whitespace_begin(recbuf,bytes_written)+1;
        recbuf[endarr-1] = 0;
        act_on_message(con, recbuf,endarr);
    } else {
        con.return_error();
    }
}

//the connection and server classes are mostly taken from the library documentation
//I am fairly sure the shared objects are threadsafe, but not 100%.
class tcp_con
    : public boost::enable_shared_from_this<tcp_con> {
public:
    typedef boost::shared_ptr<tcp_con> pointer;

    static pointer create(asio::io_service& io_service, safe_cache* cache)
    {
        return pointer(new tcp_con(io_service, cache));
    }
    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        asio::async_read(socket_, asio::buffer(buf,bufsize),
            boost::bind(&tcp_con::handle_read, shared_from_this(), asio::placeholders::error(), asio::placeholders::bytes_transferred()));
    }
    void write_message(char * str, size_t size)
    {
        asio::async_write(socket_, asio::buffer(str,size),
            boost::bind(&tcp_con::handle_write, shared_from_this()));
    }
    void return_error()
    {
        write_message(errcstr,sizeof(errcstr));
    }
    cache_t& cache()
    {
        return port_cache->impl;
    }
    tcp_con(asio::io_service& io_service, safe_cache* in_cache)
        : socket_(io_service)
        , port_cache(in_cache)
    {
    }

    void handle_read(const asio::error_code& error,
        size_t bytes_written)
    {
        if (error == asio::error::eof) {
            process_message(*this,buf,bytes_written);
        }
        else if (error)
        {
            throw asio::system_error(error);
        }
        else
        {
            throw runtime_error("did not find end of file before buffer end");
        }
    }
    void handle_write()
    {
    }

    tcp::socket socket_;
    safe_cache* port_cache; //non-owning
    char buf[bufsize];
};

class tcp_server {
public:
    tcp_server(asio::io_service& io_service, int portnum, safe_cache& in_cache)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), portnum))
        , port_cache(&in_cache)
    {
    }

    void start_accept()
    {
        tcp_con::pointer new_connection = tcp_con::create(acceptor_.get_io_service(), port_cache);

        acceptor_.async_accept(new_connection->socket(),
            boost::bind(&tcp_server::handle_accept, this, new_connection,
                                   asio::placeholders::error));
    }
private:
    void handle_accept(tcp_con::pointer new_connection,
        const asio::error_code& error)
    {
        if (!error) {
            new_connection->start();
        } else {
            throw asio::system_error(error);
        }

        start_accept();
    }

    tcp::acceptor acceptor_;
    safe_cache* port_cache; //non-owning
};
class udp_server {
public:
    udp_server(asio::io_service& io_service, int udp_port, safe_cache& incache)
        : socket_(io_service, udp::endpoint(udp::v4(), udp_port))
        , port_cache(&incache)
    {
    }
    void write_message(char * str,size_t size)
    {
        //printf("%s\n%d\n",str,size);
        socket_.async_send_to(asio::buffer(str, size), endpoint,
            boost::bind(&udp_server::handle_send, this, asio::placeholders::error(), asio::placeholders::bytes_transferred()));
    }
    void return_error()
    {
        write_message(errcstr,sizeof(errcstr));
    }
    cache_t& cache()
    {
        return port_cache->impl;
    }

    void start_receive()
    {
        shared_ptr<bufarr> recbuf(new(bufarr));
        socket_.async_receive_from(asio::buffer(*recbuf), endpoint,
            boost::bind(&udp_server::handle_receive, this,recbuf,
                                       asio::placeholders::error(), asio::placeholders::bytes_transferred()));
    }
    void handle_receive(shared_ptr<bufarr> recbuf, const asio::error_code& error, size_t bytes_written)
    {
        if (!error) {
            process_message(*this,recbuf->data(),bytes_written);
        } else {
            throw asio::system_error(error);
        }
        start_receive();
    }
    void handle_send(const asio::error_code&, size_t)
    {
    }
    //these are apparently threadsafe according to stack overflow
    udp::socket socket_;
    udp::endpoint endpoint;

    safe_cache* port_cache; //non-owning
};
void run_loop(asio::io_service * service){
    try{
        service->run();
    }
    catch(ExitException&){
        //this is normal
        exit(0);
    }
    catch(exception & unexpected_except){
        cout << unexpected_except.what() << endl;
        exit(1);
    }
}
void run_server(int tcp_port_start, int num_tcp_ports, int udp_port_start, int num_udp_ports, int maxmem)
{
    asio::io_service my_io_service;
    safe_cache serv_cache(create_cache(maxmem, nullptr));

    vector<tcp_server> tcps;
    tcps.reserve(num_tcp_ports);
    vector<udp_server> udps;
    udps.reserve(num_udp_ports);
    for (int i = 0; i < num_tcp_ports; i++) {
        tcps.emplace_back(my_io_service, tcp_port_start + i, serv_cache);
        tcps.back().start_accept();
    }
    for (int i = 0; i < num_udp_ports; i++) {
        udps.emplace_back(my_io_service, udp_port_start + i, serv_cache);
        udps.back().start_receive();
    }
    int64_t num_other_threads =  THREADS_PER_VCORE * stdthread::hardware_concurrency() - 1;
    using sthread = typename stdthread::thread;
    vector<sthread> o_threads;
    o_threads.reserve(num_other_threads);
    //pulled from asio documentation on how to multithread in ASIO
    for(int64_t t_n = 0; t_n < num_other_threads; t_n++){
        o_threads.emplace_back(run_loop,&my_io_service);
    }
    //infinite loop
    run_loop(&my_io_service);
}

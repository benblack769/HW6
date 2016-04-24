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

using stdthread = std::thread;
using namespace std;

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
    int num_tcps = 130;
    int num_udps = 130;
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
    try {
        run_server(tcp_portnum, num_tcps, udp_portnum, num_udps, maxmem);
    } catch (ExitException&) {
        //this is normal, do nothing
    }
    //catch(exception & unexpected_except){
    //    cout << unexpected_except.what();
    //   return 1;
    //}

    return 0;
}
string make_json(string key, string value)
{
    return "{ \"key\": \"" + key + "\" , \"value\": \"" + value + "\" } ";
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
void get(con_ty& con, string key)
{
    uint32_t val_size = 0;
    val_type v = cache_get(con.cache(), (key_type)(key.c_str()), &val_size);
    static uint64_t num_total = 0;
    static uint64_t num_hits = 0;
    num_total++;
    if (v != nullptr) {
        string output = make_json(key, string((char*)(v)));
        con.write_message(output);
        num_hits++;
    } else {
        con.return_error();
    }
}
template <typename con_ty>
void put(con_ty& con, string key, string value)
{
    cache_set(con.cache(), (key_type)(key.c_str()), (void*)(value.c_str()), value.size()+1);
}
template <typename con_ty>
void delete_(con_ty& con, string key)
{
    cache_delete(con.cache(), (key_type)(key.c_str()));
}
template <typename con_ty>
void head(con_ty&)
{
    //todo:implement!
}
template <typename con_ty>
void post(con_ty& con, string post_type, string extrainfo)
{
    strip(post_type);
    if (post_type == "shutdown") {
        throw ExitException();
    } else if (post_type == "memsize") {
        if (cache_space_used(con.cache()) == 0) {
            destroy_cache(con.cache());
            con.cache() = create_cache(stoll(extrainfo), NULL);
        } else {
            con.return_error();
        }
    } else {
        throw runtime_error("bad POST message");
    }
}
template <typename con_ty>
void act_on_message(con_ty& con, string message)
{
    size_t end_of_line = min(min(message.find('\n'), message.find('\r')), message.size() + 1);

    size_t first_space = message.find(' ');
    size_t first_slash = message.find('/', first_space + 1);
    size_t second_slash = message.find('/', first_slash + 1);

    auto begining = message.begin();
    string fword = string(begining, begining + first_space);
    string info1 = string(begining + first_slash + 1, begining + min(second_slash, end_of_line));
    string info2 = second_slash == string::npos ? string() : string(begining + second_slash + 1, begining + end_of_line);

    strip(fword);
    strip(info1);
    strip(info2);

    if (fword == "GET") {
        get(con, info1);
    } else if (fword == "PUT") {
        put(con, info1, info2);
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
//the connection and server classes are mostly taken from the library documentation
//note that they are threadsafe as long as multiple clients are not connecting to the same ports
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
        asio::async_read(socket_, asio::buffer(b),
            boost::bind(&tcp_con::handle_read, shared_from_this(), asio::placeholders::error(), asio::placeholders::bytes_transferred()));
    }
    void write_message(string s)
    {
        asio::async_write(socket_, asio::buffer(s),
            boost::bind(&tcp_con::handle_write, shared_from_this()));
    }
    void return_error()
    {
        write_message("ERROR");
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
            string s(b.begin(), b.begin() + bytes_written);
            strip(s);
            act_on_message(*this, s);
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
    bufarr b;
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
    void write_message(string s)
    {
        socket_.async_send_to(asio::buffer(s.c_str(), s.size() + 1), endpoint,
            boost::bind(&udp_server::handle_send, this, asio::placeholders::error(), asio::placeholders::bytes_transferred()));
    }
    void return_error()
    {
        write_message("ERROR");
    }
    cache_t& cache()
    {
        return port_cache->impl;
    }

    void start_receive()
    {
        socket_.async_receive_from(asio::buffer(recbuf), endpoint,
            boost::bind(&udp_server::handle_receive, this,
                                       asio::placeholders::error(), asio::placeholders::bytes_transferred()));
    }
    void handle_receive(const asio::error_code& error, size_t bytes_written)
    {
        if (!error) {
            size_t endloc = find_in_buf(recbuf, char(0));
            if (endloc != string::npos) {
                string act_str(recbuf.begin(), recbuf.begin() + endloc);
                strip(act_str);
                act_on_message(*this, act_str);
            } else {
                return_error();
            }
        } else {
            throw asio::system_error(error);
        }
        start_receive();
    }
    void handle_send(const asio::error_code&, size_t)
    {
    }
    udp::socket socket_;
    udp::endpoint endpoint;
    safe_cache* port_cache; //non-owning
    bufarr recbuf;
};

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
    int64_t num_other_threads = stdthread::hardware_concurrency() - 1;
    using sthread = typename stdthread::thread;
    /*vector<sthread> o_threads;
    o_threads.reserve(num_other_threads);
    //pulled from asio documentation on how to multithread in ASIO
    for(int64_t t_n = 0; t_n < num_other_threads; t_n++){
        o_threads.emplace_back(boost::bind(&asio::io_service::run, &my_io_service));
    }*/
    my_io_service.run();
}

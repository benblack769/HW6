#include <iostream>
#include <string>
#include "cache.h"
#include "user_con.hpp"

using namespace std;

using namespace asio;
using namespace asio::ip;

using asio::ip::tcp;

string tcp_port = "10700";
string udp_port = "10900";
string serv_name = "localhost";//"134.10.103.234";

class tcp_connection{
public:
    tcp::socket socket;
    tcp_connection(asio::io_service & service,ip::tcp::resolver::iterator & resit):
        socket(service){

        //hackish fix to a weird bind error
        asio::error_code error;
        do{
            asio::connect(socket, resit, error);
            if(error && error.value() != linux_specifc__address_not_available__error){
                throw asio::system_error(error);
            }
        }
        while(error);
    }

    char * get_message(bufarr & recbuf){
        asio::error_code error;
        size_t length = socket.receive(asio::buffer(recbuf));

        if(length >= bufsize){
            return errcstr;
        }
        else{
            recbuf[length] = 0;
            return &recbuf[0];
        }
    }
    void write_message(char * message,size_t size){
        socket.send(asio::buffer(message,size));
    }
};

class sock_opt_h{
public:
    timeval time;
    sock_opt_h(){
        time.tv_sec = 0;
        time.tv_usec = 50 * 1000;
    }
    int level(const udp &) const{
        return SOL_SOCKET;
    }
    int name(const udp &)const{
        return SO_RCVTIMEO;
    }
    void * data(const udp &)const{
        return (void*)(&time);
    }
    //const timeval &
    size_t size(const udp &)const{
        return sizeof(time);
    }
};
class udp_connection{
public:
    udp::socket socket;

    udp_connection(asio::io_service & service,udp::endpoint & reciver):
        socket(service){

        socket.connect(reciver);
        set_block_timeout();
    }
    void set_block_timeout(){
        sock_opt_h def;
        socket.set_option(def);
    }
    char * get_message(bufarr & recbuf){
        bufarr buf;
        size_t length = socket.receive(asio::buffer(recbuf));

        if(length >= bufsize){
            return errcstr;//todo: make a valid error code
        }
        else{
            recbuf[length] = 0;
            return recbuf.data();
        }
    }
    void write_message(char * str,size_t size){
        socket.send(asio::buffer(str,size));
    }
};
struct cache_obj{
    ip::tcp::resolver::iterator resit;
    udp::endpoint reciver;
    asio::io_service my_io_service;
    ip::tcp::resolver tcp_resolver;
    ip::udp::resolver udp_resolver;

    bufarr sendarr;

    cache_obj():
        my_io_service(),
        tcp_resolver(my_io_service),
        udp_resolver(my_io_service)
    {
        resit = tcp_resolver.resolve({serv_name, tcp_port});
        reciver = *udp_resolver.resolve({serv_name, udp_port});
    }
    size_t make_sendarr(const char * head,char * word1,char * word2){
        size_t headlen = strlen(head);
        size_t w1len = strlen(word1);
        size_t w2len = word2 == nullptr ? 0 : strlen(word2);
        if(headlen + w1len + w2len + 5 > bufsize){
            throw runtime_error("data too big for send");
        }
        else{
            auto arrit = sendarr.begin();
            arrit = copy(head,head+headlen,arrit);
            *arrit = ' '; arrit++;
            *arrit = '/'; arrit++;
            arrit = copy(word1,word1+w1len,arrit);
            if(word2 != nullptr){
                *arrit = '/'; arrit++;
                arrit = copy(word2,word2+w2len,arrit);
            }
            *arrit = char(0); arrit++;
            for(int i = 0; i < 1000; i++)
                *arrit = char(0); arrit++;

            size_t length =  &*arrit - &*sendarr.begin();
            return length;
        }
    }
    char * send_message_tcp(bool getmes,const char * head,char * word1,char * word2=nullptr){
        tcp_connection con(my_io_service,resit);
        size_t tot_len = make_sendarr(head,word1,word2);
        con.write_message(&sendarr[0],tot_len);
        return getmes ? con.get_message(sendarr) : nullptr;
    }
    char * send_message_udp(bool getmes,const char * head,char * word1,char * word2=nullptr){
        udp_connection con(my_io_service,reciver);
        size_t tot_len = make_sendarr(head,word1,word2);
        con.write_message(&sendarr[0],tot_len);
        return getmes ? con.get_message(sendarr) : nullptr;
    }
};
size_t find(char * firststr,size_t strlen,size_t startpos,char c,uint64_t count){
    uint64_t mycount = 0;
    for(size_t i = startpos; i < strlen; i++){
        if(firststr[i] == c){
            mycount++;
            if(mycount == count){
                return i;
            }
        }
    }
    return string::npos;
}
void unpack_json(char * json_str,char *& key, char *& value){
    size_t len = strlen(json_str);

    size_t startkey = find(json_str,len,0,'"',3) + 1;
    size_t endkey = find(json_str,len,startkey,'"',1);

    size_t startval = find(json_str,len,endkey+1,'"',3) + 1;
    size_t endval = find(json_str,len,startval,'"',1);

    json_str[endkey] = 0;
    json_str[endval] = 0;

    key = json_str + startkey;
    value = json_str + startval;
}
cache_t get_cache_connection(){
    return new cache_obj();
}
cache_t create_cache(uint64_t maxmem,hash_func ){
    string maxmemstr= to_string(maxmem);
    cache_t outc = new cache_obj();
    outc->send_message_tcp(false,"POST","memsize",(char *)(maxmemstr.c_str()));
    return outc;
}
void cache_set(cache_t cache, key_type key, val_type val, uint32_t val_size){//val_size is ignored
    if(val_size > bufsize-1){
        return;
    }
    char * val_str = (char *)(val);//val is assumed to be a string
    cache->send_message_tcp(false,"PUT",(char*)(key),val_str);
}
val_type cache_get(cache_t cache, key_type key, uint32_t *val_size){
    *val_size = 0;

    char * keystr = ((char*)key);
    char * retval = cache->send_message_udp(true,"GET",keystr);
    if(retval == nullptr || strcmp(errcstr,retval) == 0){
        return nullptr;
    }
    else{
        char * valstr;
        char * retkeystr;
        unpack_json(retval,retkeystr,valstr);

        if(strcmp(keystr,retkeystr)){
            //this is a network logic error
            cout << "key failed:\n" << keystr <<  endl << retkeystr << endl << endl;
            return nullptr;
        }
        return valstr;//this is fine because valstr is stored in the protocol buffer.
    }
}
void cache_delete(cache_t cache, key_type key){
    cache->send_message_tcp(false,"DELETE",(char*)key);
}
uint64_t cache_space_used(cache_t cache){
    return 0;//not implemented
}
void destroy_cache(cache_t cache){
    cache->send_message_tcp(false,"POST","shutdown");
    delete cache;
}
void end_connection(cache_t cache){
    delete cache;
}
//this is a helper function for the timing code that allows me
// to see what server is spending its time on
void call_head_no_return(cache_t cache){
    cache->send_message_tcp(false,"HEAD","null");
}

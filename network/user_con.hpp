#pragma once
#include <asio.hpp>
#include <vector>
#include <array>
#include <unistd.h>
//#include <bind/bind.hpp>
//#include <ctime>
#include <vector>

using namespace asio;
using namespace asio::ip;

const int linux_specifc__address_not_available__error = 99;

constexpr size_t bufsize = (1 << 16) - 32;
char errcstr[] = "ERROR";
std::string errstr = "ERROR";
//constexpr size_t MIN_TCP_SIZE = 1024;//must be less than bufsize

//class udp_connection;

//void handle_receive(const asio::error_code& ec, std::size_t length,
//                    udp_connection * con);
using bufarr = std::array<char,bufsize>;

template<typename bufty>
size_t find_in_buf(bufty & buf,char c){
    for(size_t pos = 0; pos < bufsize; pos++){
        if(buf[pos] == c){
            return pos;
        }
    }
    return std::string::npos;
}
bool is_white_space(char c){
    //first one is non-standard, but useful in my application
    return c == char(0) || c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

size_t whitespace_begin(char * str,size_t size){
    for(int i = size-1; i >= 0;i--){
        if(!is_white_space(str[i])){
            return i + 1;
        }
    }
    return 0;
}
void strip(std::string & str){
    size_t strs = str.size();
    for(int i = strs-1; i >= 0;i--){
        if(!is_white_space(str[i])){
            str.resize(i+1);
            break;
        }
    }
}

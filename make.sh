rm server*
rm client_time*
gcc -std=c99  -I "impl/" -c impl/hash_cache.c impl/helper.c impl/lru_replacement.c
#WIN_BOOST_LINK="-L C:/boost_1_60_0/lib_gcc"
#MY_WINDOWS_LINK_LIBS=" -lws2_32 -lwsock32 -lboost_system-mgw52-mt-1_60 -lboost_date_time-mgw52-mt-1_60"
g++ -std=c++11 -pthread $WIN_BOOST_LINK -w  -I "network/" -I "impl/" -I "asio/include/" -o server hash_cache.o helper.o lru_replacement.o network/server.cpp $MY_WINDOWS_LINK_LIBS
g++ -std=c++11 -pthread $WIN_BOOST_LINK -w -I "network/" -I "test/" -I "asio/include/" -o client_time network/client.cpp time_cache.cpp $MY_WINDOWS_LINK_LIBS

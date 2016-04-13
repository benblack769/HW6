gcc -std=c99  -O3 -I "impl/" -c impl/hash_cache.c impl/helper.c impl/lru_replacement.c
g++ -std=c++11 -O3 -pthread -Wall -I "network/" -I "impl/" -I "asio/include/" -o server hash_cache.o helper.o lru_replacement.o network/server.cpp
g++ -std=c++11 -O3 -pthread -Wall -I "network/" -I "test/" -I "asio/include/" -o client_time network/client.cpp time_cache.cpp

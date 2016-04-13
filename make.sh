gcc -std=c99  -O3 -I "impl/" -c impl/hash_cache.c impl/helper.c impl/lru_replacement.c
g++ -std=c++11 -O3 -pthread -Wall -I "impl/" -I "asio/include/" -o server hash_cache.o helper.o lru_replacement.o server.cpp
g++ -std=c++11 -O3 -pthread -Wall -I "test/" -I "asio/include/" -o client_tests  client.cpp time_cache.cpp

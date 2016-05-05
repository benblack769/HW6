#WIN_BOOST_LINK="-L C:/boost_1_60_0/lib_gcc"
#MY_WINDOWS_LINK_LIBS=" -lws2_32 -lwsock32 -lboost_system-mgw52-mt-1_60 -lboost_date_time-mgw52-mt-1_60"
g++ -O3 -std=c++11 -pthread $WIN_BOOST_LINK -w -I "network/" -I "impl/" -I "asio/include/" -o server impl/hash_cache.cpp impl/helper.cpp impl/lru_replacement.cpp network/server.cpp $MY_WINDOWS_LINK_LIBS
gcc -std=c99 -O3 -fpermissive -w -c test/basic_test.c test/main.c test/test.c test/test_helper.c test/lrutests.c
g++ -O3 -std=c++11 -D TEST -pthread $WIN_BOOST_LINK -w -I "network/" -I "test/" -I "asio/include/" -o client_tests network/client.cpp  main.o  lrutests.o basic_test.o  test_helper.o test.o $MY_WINDOWS_LINK_LIBS

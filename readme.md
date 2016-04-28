## Multi-threading: HW7

ASIO really came through for me here. It has a very specific idea of how to make a multi-threaded networking code, which actually works extremely well. I realize in hindsight that a lot of the pain I went through in the earlier assignments which was completely unnecessary for a single threaded server was nearly perfect code for a multithreaded server. So making the networking stuff multithreaded was very easy, just calling io_service.run() in a few threads, and making sure that buffers were not shared between threads.

On the cache side, I locked the cache top to bottom and got substantially better performance, as I will show later, so I did not feel the need to fine grain lock it (and that would be too hard).

### Bugs

Every performance run was also a test for basic correctness, measuring hit rate and whether the value returned is what I expected. On occasion, the following buggy behavior occurred.

* Crashes at startup with segfault
* Crashes at startup with floating point exception (I don't think I use floating points in my server at all).
* Hit rate drops to 0.5% on some runs (I threw out all of this data).
* About every 100000th successful get, the server spits out random garbage.

The last two definitely did not happen before I made the cache multi-threaded, so I am clearly not keeping something safe I really should.  I am not sure about how safe the ASIO sockets, acceptors and other objects are (io_service is definitely safe), so I may have made a bad assumption there.

## Optimization: HW8

This was a great exercise in how to not optimize a cache. My very first idea was, the client takes more CPU than the server, so lets optimize it! So I made string parsing much more efficient. This did not affect performance in any noticeable way.

After that failure I decided to start measuring the cache. Unfortunately in all of my profiles, the cache tended to have very different results, especially for the most expensive function.

So here is a table of gprof profile files with associated compiler options and commit level (ordered by when I actually ran the test):

Profile filenames | Options | Optimization Commit name | most expensive function
--- | --- | --- | ---
gprofdata.txt | g++ -Og -pg | global cache lock finished | querry_hash
gprofdata1.txt | g++ -Og -pg | global cache lock finished | assign_to_link
gprofdata2.txt | g++-5 -O3 -pg | global cache lock finished | info_gotten
o3sing.txt | g++ -O3 -pg | global cache lock finished(single threaded) | info_gotten
o3flto.txt | g++-5 -O3 flto -pg |  global cache lock finished | querry_hash
linkopt1.txt | g++-5 -O3 flto -pg | link optimization finished | void get<udp_server>
lruopt.txt | g++-5 -O3 flto -pg | lru optimization tested |  void get<udp_server>

The initial profiles did not paint a very clear picture of performance, however, I did notice a string operation near the of some of the lists (but at the top of none of them), so I decided to optimize away string copies. I also noticed that querry_hash did appear at the top of several lists, and def_hash_fn appeared in the top 5 in nearly every early list, so I decided to reduce the number of querry_hash calls.

### Optimizations

Optimizations are referred to by their commit name. Here is a table for reference.

Commit name | optimization description
--- | ---
global cache lock finished | multithreaded the cache
string optimization now fully working | removed excessive string copies
link optimization finished | made hash table a doubly linked list and removed a querry_hash call from cache_set and .
lru optimization tested | made lru interface much more clean and sensible.

### Multiple Clients

I spent a lot of time configuring my client to work nicely with other clients while timing the cache, and ran some preliminary runs with multiple clients communicating with the server over different ports but ended up not using it because:

1. It was a lot harder to analyze the throughput and make sure it is good data.
2. It gave much, much more inconsistent data.
3. A single client seemed to saturate the server network (see network problem).

### Workload

I ran into weird bugs trying to get the key and value sizes to 8 and 16 bytes, respectively, so I just went with my old workload I made for HW8. This uses much larger values and keys in general, and seems much slower on preliminary tests on localhost than the prescribed sizes.

I believe there are around 21000 key value pairs that the timing system keeps track of and sends to the cache, and the value sizes total around 10 million. So almost everything in the cache can probably fit into the L3 cache on those machines, and certainly all the cache metadata. So this may not be terribly realistic or comparable to other cache implementations, but weird bugs make it impossible to set too many values to the cache in the beginning.

### Assumptions about performance

The last homework, seemed to show that there was not an important difference in throughput between numbers of threads after a certain point, so I just set a fixed number of threads, and assumed that it would be about the same for different number of threads (and correspondingly, different latencies). Tests seemed to show this did not really matter either.

### Run system

All profiles, preliminary, and the single machine measurements happened on my laptop compiling with g++ 5.3 with link time optimization enabled. The two machine of measurements listed here are measured using the most recent version of time_cache.cpp and are compiled with g++ 4.8, without link time optimization or profiling and run with the server on mandu.

I ran all of the official timings after I wrote all of the code. I simply went back through commits, changed the make.sh script to be consistent, compiled and ran the server, while using the same binary for the client throughout all timings.

### Two machine analysis

The raw data as output by time_cache.cpp is in two_mach_data.txt.

As expected, the performance increased dramatically with multithreading, nearly doubling throughput. However subsequent optimizations did not really help at all.

Looking at the output of top, I discovered that my server was not using all of the CPU (about 2.5 cores out of 4). In addition, a process called ksoftirqd sprung up whenever I ran my server and used about 75% of a single CPU (this increases to 100% with multiple clients). Apparently it is the daemon that deals with network packets, and it looks like it was completely overwhelmed, so no matter how fast my server ran, that would be the bottleneck. So I optimized for the completely wrong thing, I was optimizing for CPU/memory performance when I really should have been optimizing for network performance.

The lesson here is that profiling a network program on localhost does not accurately find bottlenecks of the program as a whole.

### Raw data on a single machine

Just to actually see the result of my hard earned optimizations, I also ran all the server versions on localhost. Note that this data is not really comparable to the two machine version due to the lack of network daemon bottlenecks and the presence of a high CPU usage client.

Here the performance actually did increase quite a bit for most of the optimizations, as expected. This was rather comforting to know. The lru optimization did not affect performance, but it made the code much cleaner, so it was an improvement anyways.

The final profile, lruopt.txt, showed that gets, not sets, are the biggest bottleneck, and there is no obvious redundant calculations there, so the next logical CPU/memory optimization would to remove the large lock on cache_get.


### Run system

All profiles and preliminary measurements happened on my laptop compiling with g++ 5.3 with link time optimization enabled. All full measurements listed here are measured using the most recent version of time_cache.cpp and are compiled with g++ 4.8, without link time optimization or profiling and run with the server on mandu and the client on banku. I did notice a substantial performance difference between machines in the polytopia lab, which I found odd.

I spent a lot of time configuring my client to work nicely with other clients while timing the cache, and ran some preliminary runs with multiple clients communicating with the server over different ports but ended up not using it because:

1. It was a lot harder to analyze the throughput and make sure it is good data.
2. It gave much, much more inconsistent data.
3. A single client seemed to saturate the server (looking at CPU percentages).

### Workload

I ran into weird bugs trying to get the key and value sizes to 8 and 16 bytes, respectively, so I just went with my old workload I made for HW8. This uses much larger values and keys in general, and seems much slower on preliminary tests on localhost than the prescribed sizes.

I believe there are around 21000 key value pairs that the timing system keeps track of and sends to the cache, and the value sizes total around 10 million. So almost everything in the cache can probably fit into the L3 cache on those machines, and certainly all the cache metadata. So this may not be terribly realistic or comparable to other cache implementations, but weird bugs make it impossible to set too many values to the cache in the beginning.

## Multi-threading: HW7

ASIO really came through for me here. It has a very specific idea of how to make a multi-threaded networking code, which actually works extremely well. I realize in hindsight that a lot of the pain I went through in the earlier assignments which was completely unnecessary for a single threaded server was nearly perfect code for a multithreaded server. So making the networking stuff multithreaded was very easy, just calling io_service.run() in a few threads, and making sure that buffers were not shared between threads.

I locked the cache top to bottom and got substantially better

## Optimization: HW8

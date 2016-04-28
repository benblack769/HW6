
### Run system

All profiles and preliminary measurements happened on my laptop compiling with g++ 5.3 with link time optimization enabled. All full measurements listed here are measured using the most recent version of time_cache.cpp and are compiled with g++ 4.8, without link time optimization or profiling and run with the server on mandu.

I spent a lot of time configuring my client to work nicely with other clients while timing the cache, and ran some preliminary runs with multiple clients communicating with the server over different ports but ended up not using it because:

1. It was a lot harder to analyze the throughput and make sure it is good data.
2. It gave much, much more inconsistent data.
3. A single client seemed to saturate the server (looking at CPU percentages).

### Workload

I ran into weird bugs trying to get the key and value sizes to 8 and 16 bytes, respectively, so I just went with my old workload I made for HW8. This uses much larger values and keys in general, and seems much slower on preliminary tests on localhost than the prescribed sizes.

I believe there are around 21000 key value pairs that the timing system keeps track of and sends to the cache, and the value sizes total around 10 million. So almost everything in the cache can probably fit into the L3 cache on those machines, and certainly all the cache metadata. So this may not be terribly realistic or comparable to other cache implementations, but weird bugs make it impossible to set too many values to the cache in the beginning.

## Multi-threading: HW7

ASIO really came through for me here. It has a very specific idea of how to make a multi-threaded networking code, which actually works extremely well. I realize in hindsight that a lot of the pain I went through in the earlier assignments which was completely unnecessary for a single threaded server was nearly perfect code for a multithreaded server. So making the networking stuff multithreaded was very easy, just calling io_service.run() in a few threads, and making sure that buffers were not shared between threads.

On the cache side, I locked the cache top to bottom and got substantially better performance.

I tested my cache the entire time to ensure correctness, by checking the keys and values that were returned and seeing if they were what I was expecting. After I implemented multithreading, about 1/100000th of the time, the cache spits out complete garbage. I suspect that valgrind could help me solve this problem, but I did not have time to look into what caused this bug. I also kept track of gets, and noticed that if I start more than a certain number of threads on the server side, the hit rate drops from 90% to about 5%, which is inexplicable.

## Optimization: HW8

This was a great exercise in how to not optimize a cache. My very first idea was, the client takes more CPU than the server, so lets optimize it! So I made string parsing much more efficient. This did not affect performance in any noticeable way.

After that failure I decided to start measuring the cache. Unfortunately in all of my profiles, the cache tended to

Commit: Confirmed gets are working

number of threads = 97
average time in ms = 0.825213
average throughput = 117545
hit rate = 0.902062

number of threads = 97
average time in ms = 0.893702
average throughput = 108537
hit rate = 0.901158

number of threads = 97
average time in ms = 1.06236
average throughput = 91305.9
hit rate = 0.901742

Commit: multiclient is now working smoothly

number of threads = 97
average time in ms = 0.498722
average throughput = 194497
hit rate = 0.901311


number of threads = 97
average time in ms = 0.495417
average throughput = 195795
hit rate = 0.90189


number of threads = 97
average time in ms = 0.489305
average throughput = 198240
hit rate = 0.902229

Commit: String optimization fully working

number of threads = 97
average time in ms = 0.496076
average throughput = 195535
hit rate = 0.901163

number of threads = 97
average time in ms = 0.493701
average throughput = 196475
hit rate = 0.903093

number of threads = 97
average time in ms = 0.501616
average throughput = 193375
hit rate = 0.902022

link optimization finished

number of threads = 97
average time in ms = 0.500005
average throughput = 193998
hit rate = 0.902404

number of threads = 97
average time in ms = 0.503459
average throughput = 192667
hit rate = 0.901456

number of threads = 97
average time in ms = 0.50098
average throughput = 193621
hit rate = 0.901718

lru optimization tested

number of threads = 97
average time in ms = 0.521925
average throughput = 185850
hit rate = 0.90147

number of threads = 97
average time in ms = 0.512567
average throughput = 189243
hit rate = 0.9017

number of threads = 97
average time in ms = 0.501936
average throughput = 193252
hit rate = 0.901676

number of threads = 97
average time in ms = 0.502862
average throughput = 192896
hit rate = 0.902331

ksoftirqd

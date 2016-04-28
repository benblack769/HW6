
### Run system

All profiles and preliminary measurements happened on my laptop compiling with g++ 5.3 with link time optimization enabled. All full measurements listed here are measured using the most recent version of time_cache.cpp and are compiled with g++ 4.8, without link time optimization and run with the server on mandu and the client on banku.

### Changes to Timing Code

I measured the average value size in my random distribution to be ~460 bytes, so I made the cache have a 81% hit rate by simply having maxmem at around 81% of the number of values times the average value size.

I decreased maxmem to **???????????????** bytes to get around the weird bug, **so many of the items in the software cache will probably be in the hardware caches.**

## Multi-threading: HW7

ASIO really came through for me here. It has a very specific idea of how to make a multi-threaded networking code, which actually works extremely well. I realize in hindsight that a lot of the pain I went through in the earlier assignments which was completely unnecessary for a single threaded server was nearly perfect code for a multithreaded server. So making the networking stuff multithreaded was very easy, just calling io_service.run() in a few threads, and making sure that buffers were not shared between threads.

I locked the cache top to bottom and got substantially better

## Optimization: HW8

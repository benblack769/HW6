### Goals and System Boundaries

The goal is to measure the performance of the cache.

The system includes the operating system, the network between the server and the client, the CPU and memory of both the server and the client.

### Services and Outcomes

The services those described in cache.h. Each call to any function can take any amount of time, including unbounded time. cache_get can either return the value associated with the key or NULL.

### Metrics

Sustained throughput, the maximum offered load (in requests per second) at which the mean response time remains under 1 millisecond.

Hit rate, the number of gets which are in the cache.

Latency?

### Parameters

#### System Parameters

Operating System
Network connection between server and client
Network protocol (TCP or UDP)
Server memory
Server CPU
If swapping occurs then server storage type (HDD, SSD etc).

#### Workload Parameters

maxmem of the cache on the server.

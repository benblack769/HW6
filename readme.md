### 1. Goals and System Boundaries

The goal is to measure the performance of the cache.

The system includes the operating system, the network between the server and the client, the CPU and memory of both the server and the client.

### 2. Services and Outcomes

The services those described in cache.h. Each call to any function can take any amount of time, including unbounded time. cache_get can either return the value associated with the key or NULL.

### 3. Metrics

Sustained throughput, the maximum offered load (in requests per second) at which the mean response time remains under 1 millisecond.

Hit rate, the number of gets which are in the cache.

Latency?

### 4. Parameters

#### System Parameters

Network connection between server and client
Network protocol (TCP or UDP)
Operating System of server
Server memory
Server CPU
If swapping occurs frequently then server storage type (HDD, SSD etc).

#### Workload Parameters

maxmem of the cache on the server.
number of items in cache (as opposed to their total size like maxmem)
size of requests
type of requests (get, set, delete)
number of requests of each type per second period

### 6. Factors to study

Operating system?
Network protocol

### 7. Workload

Distribution of requests (roughly approximating distribution found with ETC in the memcache paper):
30% delete
3% set/update
67% get

Distribution of key size:
Normal distribution with mean = 30, standard deviation = 8 as from paper

Distribution of value size.

Uniform distribution until 500 bytes, that will account for most of the requests, as seems to be roughly the case from the ETC Value size chart.

For large sizes, there seems to be a uniform distribution of work per size. So

### 8.

### 1. Goals and System Boundaries

The goal is to measure the performance of the cache.

The system includes the operating system, the network between the server and the client, the CPU and memory of both the server and the client.

### 2. Services and Outcomes

The services those described in cache.h. Each call to any function can take any amount of time, including unbounded time. cache_get can either return the value associated with the key or NULL. Since it passes the tests, I will assume that if it returns non-null, it returns the correct answer.

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

Distribution of requests | Explanation
--- | ---
30% set deleted value | ???
30% delete value | ???
3% update old | ???
67% get item | Around what memcache did

These are a rough approximation of the distribution found with ETC in the memcache paper. It is not perfect since it is not clear how new values are brought into the cache from the database. Presumably, that happens when get is called, so.

#### Distribution of key size

Normal distribution with mean = 30, standard deviation = 8 as from paper

#### Distribution of value size

90% go to a uniform distribution between 2 and 500

10% go to a 10^(-2.341611959*10^-4 * n) distribution where 500 < n <= 30000

##### Explanation

Uniform distribution until 500 bytes as seems to be roughly the case from the ETC Value Size PDF chart.

The other distribution is a exponential function that matches the following two points taken from the PDF chart.

    f(501) = 10e-4
    f(30000) = 10e-7

Plugging these numbers into an exponential fit function, and viola.

    f(x) = 1.124210035 * 10^(-4) * 10^(-2.341611959*10^-4 * x)

The graph of which looks fairly similar to the data in the memcache paper.

### 8. Experimental Design









s

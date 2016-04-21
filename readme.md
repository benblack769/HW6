### 1. Goals and System Boundaries

The goal is to measure the performance of the cache.

The system includes the operating system, the network between the server and the client, the CPU and memory of both the server and the client.

### 2. Services and Outcomes

The services those described in cache.h. Each call to any function can take any amount of time, including unbounded time. cache_get can either return the value associated with the key or NULL. Since it passes the tests, I will assume that if it returns non-null, it returns the correct answer.

### 3. Metrics

Sustained throughput, the maximum number of blocking calls are processed by the cache (in requests per second) at which the mean latency remains about 1 millisecond.

Hit rate, the number of gets which are in the cache.

For, UDP, the package loss rate, the number of gets which are in the cache, but the UDP packet is lost or takes too long. This will not be studied due to time limitations.

### 4. Parameters

#### System Parameters

Network connection between server and client
Network protocol (TCP or UDP)
Operating System of server
Server memory
Server CPU
If swapping occurs frequently then server storage type (HDD, SSD etc).
Server code
Client code (impacts time from package received to value returned).

#### Workload Parameters

maxmem of the cache on the server.
number of items in cache (as opposed to their total size like maxmem)
size of requests
type of requests (get, set, delete)
number of requests of each type per second time period

### 6. Factors to study

Network protocol (for gets only)

### 7. Workload

#### Distribution of requests

* 5% set/update value
* 1% delete value
* 94% get item

These are a very rough approximation of the distribution found with ETC in the memcache paper, noting that most of the deletes in the paper are not actually in the cache, and so these are ignored.

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

This is the graph of 100,000,000 values randomly distributed over this distribution.

![alt text](https://github.com/weepingwillowben/HW6/blob/master/fit.png "random value graph")

It does not look too dissimilar  to the data in the memcache paper.

#### Size of Cache

Presumably, the memory of the cache is valuable, so I tried to use about one gigabyte for the cache, as to model a fully loaded server while avoiding swapping. Unfortunately, this turned out to be impossible, due to a weird bug where the client crashed if I tried to send too many set requests at the beginning. So the cache is slowly filling up as the test is progressing, which means that most of the gets are of null values, and there will be more hits when there are more threads. So the data is not as representative as I would hope.

#### Temporal Distribution of Requests

This is excessively difficult to model properly and so I will assume a uniform temporal distribution and will make maxmem large enough to store all keys. This will limit the implications of the study to gets which are found, and reside in main memory. Caches will not help with accesses being spread randomly over a gigabyte.

### 8. Experimental Design

#### Implementation Details

The cache is created, then a whole bunch of threads are created on the client side, call to the cache, and the time elapsed is measured.

TCP vs UDP and the number of threads are the only factors. The  







s

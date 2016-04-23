#include "cache.h"
#include <string.h>
#include <time.h>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <thread>
#include <iostream>
#include <array>
#include <algorithm>

using namespace std;

constexpr uint64_t MILS_PER_NANO = 1000000;

constexpr int64_t tcp_start = 10700;
constexpr int64_t udp_start = 10900;


uint64_t get_time_ns();
discrete_distribution<uint64_t> init_dist();

constexpr uint64_t ns_in_s = 1000000000ULL;
constexpr uint64_t mem_to_store = 100000ULL;
constexpr uint64_t maxmem = 0.81 * mem_to_store;
constexpr double APROX_MEAN_WEIGHTED_VALUE_SIZE = 461.258;//measured with unig.cpp
constexpr uint64_t tot_num_items = uint64_t(mem_to_store / APROX_MEAN_WEIGHTED_VALUE_SIZE);
constexpr size_t NUM_THREADS = 125;//one minus the number of ports
constexpr uint64_t num_actions = 5000000 / NUM_THREADS;

string values[tot_num_items];
string keys[tot_num_items];

using gen_ty = std::default_random_engine;

uint64_t rand_key_size(gen_ty & generator){
	normal_distribution<double> norm_dsit(30,8);
	return max(int64_t(2),int64_t(norm_dsit(generator)));
}

uint64_t rand_val_size(gen_ty & generator){
	static discrete_distribution<uint64_t> val_dist = init_dist();
	//todo add large values to distribution
	return val_dist(generator);
}
char rand_char(gen_ty & generator){
    uniform_int_distribution<char> lower_lettters_dist(97,96+26-1);//lower case letters
	return lower_lettters_dist(generator);
}
uint64_t uniform_rand_item(gen_ty & generator){
	uniform_int_distribution<uint64_t> unif_dist(0,tot_num_items-1);
	return unif_dist(generator);
}
string rand_str(gen_ty & generator,uint64_t size){
	string str(size,' ');
    for(uint64_t i = 0; i < size; i++){
		str[i] = rand_char(generator);
	}
	return str;
}
key_type to_key(string & val){
	return reinterpret_cast<key_type>(val.c_str());
}
val_type to_val(string & val){
	return reinterpret_cast<val_type>(val.c_str());
}
void init_values(gen_ty & generator,uint64_t num_items){
	for(uint64_t i = 0; i < num_items; i++){
		values[i] = rand_str(generator,rand_val_size(generator));
	}
}
void init_keys(gen_ty & generator,uint64_t num_items){
	for(uint64_t i = 0; i < num_items; i++){
		//the chance that keys will not be unique is minimal, and so
		//should not affect performance that much and so I will ignore it.
		keys[i] = rand_str(generator,rand_key_size(generator));
	}
}
template<typename timed_fn_ty>
uint64_t timeit(timed_fn_ty timed_fn){
	uint64_t start = get_time_ns();
	timed_fn();
	return get_time_ns() - start;
}
uint64_t get_action(gen_ty & generator,cache_t cache){
	uint64_t get_item = uniform_rand_item(generator);
	string & item = keys[get_item];
	return timeit([&](){
		uint32_t null_val = 0;
		cache_get(cache,to_key(item),&null_val);
	});
}
uint64_t delete_action(gen_ty & generator,cache_t cache){
	string & item = keys[uniform_rand_item(generator)];
	return timeit([&](){
		cache_delete(cache,to_key(item));
	});
}
uint64_t set_action(gen_ty & generator,cache_t cache){
	uint64_t item_num = uniform_rand_item(generator);
	string & key = keys[item_num];
	string & value = values[item_num];
	return timeit([&](){
		cache_set(cache,to_key(key),to_val(value),value.size()+1);
	});
}
cache_t get_cache(int64_t tcp_portnum,int64_t udp_portnum){
	tcp_port = to_string(tcp_portnum);
	udp_port = to_string(udp_portnum);
	return get_cache_connection();
}
uint64_t rand_action_time(gen_ty & generator,cache_t cache){
	uniform_real_distribution<double> occurs_dis(0,100);//lower case letters
	double distri_val = occurs_dis(generator);

	if(distri_val < 2){
		return delete_action(generator,cache);
	}
	else if(distri_val < 7){
		return set_action(generator,cache);
	}
	else{
		return get_action(generator,cache);
	}
}
double run_rand_actions(gen_ty & generator,cache_t cache,uint64_t num_actions){
	double tot_time = 0;
	for(int i = 0; i < num_actions; i++){
		tot_time += rand_action_time(generator,cache);
	}
	return tot_time / num_actions;
}
double arr[NUM_THREADS];
void run_requests(cache_t cache,uint64_t t_num){
	gen_ty generator(t_num);
	arr[t_num] = run_rand_actions(generator,cache,num_actions);
}
double time_threads(){
	thread ts[NUM_THREADS];
	cache_t caches[NUM_THREADS];
	for(uint64_t t_n = 0; t_n < NUM_THREADS; t_n++){
		caches[t_n] = get_cache(tcp_start+t_n,udp_start+t_n);
		ts[t_n] = thread(run_requests,caches[t_n],t_n);
	}
	for(uint64_t t_n = 0; t_n < NUM_THREADS; t_n++){
		ts[t_n].join();
		end_connection(caches[t_n]);
	}
	double sum_time = 0;
	for(uint64_t t_n = 0; t_n < NUM_THREADS; t_n++){
		sum_time += arr[t_n];
	}
	return sum_time / NUM_THREADS;
}
void populate_cache(cache_t cache){
	for(size_t i = 0; i < tot_num_items; i++){
		cache_set(cache,to_key(keys[i]),to_val(values[i]),values[i].size()+1);
	}
}
int main(int argc,char ** argv){
	gen_ty generator(1);
	init_values(generator,tot_num_items);
	init_keys(generator,tot_num_items);

	uint64_t init_tcp_port = tcp_start+NUM_THREADS+1;
	uint64_t init_udp_port = udp_start+NUM_THREADS+1;

    tcp_port = to_string(init_tcp_port);
    udp_port = to_string(init_udp_port);
	cache_t cache = create_cache(maxmem,NULL);

    populate_cache(cache);

	end_connection(cache);

	double av_ms_time = time_threads() / MILS_PER_NANO;

	cout << "number of threads = " << NUM_THREADS << "\n";
	cout << "average time in ms = " << av_ms_time << "\n";
	cout << "average throughput = " <<  NUM_THREADS / (av_ms_time / 1000.0) << "\n";

	destroy_cache(get_cache(init_tcp_port,init_udp_port));//closes server

	return 0;
}

uint64_t get_time_ns(){
	struct timespec t;
	clock_gettime(0,&t);
 	return 1000000000ULL * t.tv_sec + t.tv_nsec;
}
void normalize(double to_val,vector<double>::iterator valbeg,vector<double>::iterator valend){
	double sum = 0;
	for_each(valbeg,valend,[&](double val){
		sum += val;
	});
	for_each(valbeg,valend,[=](double & val){
		val *= to_val * (1.0/sum);
	});
}
discrete_distribution<uint64_t> init_dist(){
	const uint64_t fhd = 500;
	vector<double> vals;
	vals.push_back(0);
	vals.push_back(0);
	for(uint64_t i = 2; i <= fhd; i++){
		vals.push_back(1);
	}
	for(uint64_t i = fhd+1;i <= 30000;i++){
		vals.push_back(pow(10,-2.341611959e-4 * i));
	}
	normalize(0.9,vals.begin(),vals.begin() + fhd+1);
	normalize(0.1,vals.begin()+fhd+1,vals.end());
	return discrete_distribution<uint64_t> (vals.begin(),vals.end());
}

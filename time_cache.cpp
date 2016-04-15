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

using namespace std;

constexpr size_t MAX_NUM_THREADS = 1000;
constexpr size_t MILS_PER_NANO = 1000000;

uint64_t get_time_ns();

constexpr uint64_t ns_in_s = 1000000000;
constexpr uint64_t maxmem = 4000000000;
constexpr uint64_t tot_num_items = 10000;
array<string,tot_num_items> values;
array<string,tot_num_items> keys;
std::default_random_engine generator(get_time_ns());

uint64_t rand_key_size(){
	//the chance any two keys are the same is extrememly small, so ignore it
	static normal_distribution<double> norm_dsit(30,8);
	return max(uint64_t(2),uint64_t(norm_dsit(generator)));
}
uint64_t rand_val_size(){
	//todo add large values to distribution
	static uniform_int_distribution<uint64_t> val_dist(2,500);
	return val_dist(generator);
}
char rand_char(){
	static uniform_int_distribution<char> lower_lettters_dist(96,96+26-1);//lower case letters
	return lower_lettters_dist(generator);
}
uint64_t rand_get_item(){
	//todo: change to be more representative
	static uniform_int_distribution<uint64_t> val_dist(0,tot_num_items-1);
	return val_dist(generator);
}
uint64_t rand_uniform_item(){
	static uniform_int_distribution<uint64_t> unif_dist(0,tot_num_items-1);
	return unif_dist(generator);
}
string rand_str(uint64_t size){
	string str(size,' ');
	for(uint64_t i = 0; i < size-1; i++){
		str[i] = rand_char();
	}
	return str;
}
key_type to_key(string & val){
	return reinterpret_cast<key_type>(val.c_str());
}
val_type to_val(string & val){
	return reinterpret_cast<val_type>(val.c_str());
}
void init_values(uint64_t num_items){
	for(uint64_t i = 0; i < num_items; i++){
		values[i] = rand_str(rand_val_size());
	}
}
void init_keys(uint64_t num_items){
	for(uint64_t i = 0; i < num_items; i++){
		keys[i] = rand_str(rand_key_size());
	}
}
template<typename timed_fn_ty>
uint64_t timeit(timed_fn_ty timed_fn){
	uint64_t start = get_time_ns();
	timed_fn();
	return get_time_ns() - start;
}
uint64_t get_action(cache_t cache){
	string & item = keys[rand_get_item()];
	return timeit([&](){
		uint32_t null_val = 0;
		cache_get(cache,to_key(item),&null_val);
	});
}
uint64_t delete_action(cache_t cache){
	string & item = keys[rand_uniform_item()];
	return timeit([&](){
		cache_delete(cache,to_key(item));
	});
}
uint64_t set_action(cache_t cache){
	uint64_t item_num = rand_uniform_item();
	string & key = keys[item_num];
	string & value = values[item_num];
	return timeit([&](){
		cache_set(cache,to_key(key),to_val(value),value.size()+1);
	});
}
uint64_t rand_action_time(cache_t cache){
	static uniform_real_distribution<double> occurs_dis(0,120);//lower case letters
	double distri_val = occurs_dis(generator);

	if(distri_val < 30){
		return delete_action(cache);
	}
	else if(distri_val < 50){
		return set_action(cache);
	}
	else{
		return get_action(cache);
	}
}
uint64_t run_actions(uint64_t num_actions){
	cache_t cache = get_cache_connection();
	uint64_t tot_time = 0;
	for(int i = 0; i < num_actions; i++){
		tot_time += rand_action_time(cache);
	}
	end_connection(cache);
	return tot_time / num_actions;
}
uint64_t arr[MAX_NUM_THREADS] = {0};
void run_requests(uint64_t t_num){
	arr[t_num] = run_actions(1000);
}
uint64_t time_threads(uint64_t num_threads){
	thread ts[MAX_NUM_THREADS];
	for(uint64_t t_n = 0; t_n < num_threads; t_n++){
		ts[t_n] = thread(run_requests,t_n);
	}
	for(uint64_t t_n = 0; t_n < num_threads; t_n++){
		ts[t_n].join();
	}
	uint64_t sum_time = 0;
	for(uint64_t t_n = 0; t_n < num_threads; t_n++){
		sum_time += arr[t_n];
	}
	return sum_time / num_threads;
}
int main(int argc,char ** argv){
	cache_t cache = create_cache(maxmem,NULL);

	init_values(tot_num_items);
	init_keys(tot_num_items);

	for(uint64_t n_t = 1; n_t < MAX_NUM_THREADS; n_t++){
		uint64_t av_time = time_threads(n_t);
		cout << av_time << endl;
		if(av_time > 1000000){
			break;
		}
	}

	destroy_cache(cache);

	return 0;
}

uint64_t get_time_ns(){
	struct timespec t;
	clock_gettime(0,&t);
 	return 1000000000ULL * t.tv_sec + t.tv_nsec;
}

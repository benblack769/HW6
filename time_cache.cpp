#include "cache.h"
#include <string.h>
#include <time.h>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <ctime>

using namespace std;
uint64_t get_time_ns();

constexpr uint64_t ns_in_s = 1000000000;
constexpr uint64_t maxmem = 4000000000;
constexpr uint64_t tot_num_items = 10000;
vector<string> values;
vector<string> keys;
std::default_random_engine generator(get_time_ns());

uint64_t rand_key_size(){
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
	static uniform_int_distribution<uint64_t> val_dist(0,values.size());
	return val_dist(generator);
}
uint64_t rand_uniform_item(){
	static uniform_int_distribution<uint64_t> unif_dist(0,values.size());
	return unif_dist(generator);
}
string rand_str(uint64_t size){
	string str;
	str.resize(size);
	for(uint64_t i = 0; i < size; i++){
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
		values.push_back(rand_str(rand_val_size()));
	}
}
void init_keys(uint64_t num_items){
	for(uint64_t i = 0; i < num_items; i++){
		keys.push_back(rand_str(rand_key_size()));
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

int main(int argc,char ** argv){
	bool is_master = false;
	if(argc < 2){
        printf("needs one argument\n");
        exit(1);
	}
	else if (argc == 3){
		is_master = true;
	}
	char * num_str = argv[1];
    uint64_t num_clients = strtoumax(num_str,NULL,10);

	uint64_t this_num_items = tot_num_items / num_clients;

	init_values(this_num_items);
	init_keys(this_num_items);

	cache_t cache = is_master ? create_cache(maxmem,NULL) : get_cache_connection();

	uint64_t tot_time = 0;
	for(int i = 0; i < 1000; i++){
		tot_time += rand_action_time(cache);
	}
    printf("time taken = %f\n",tot_time / double(ns_in_s));

	if(is_master){
    	destroy_cache(cache);
	}
	else{
		end_connection(cache);
	}
    return 0;
}

uint64_t get_time_ns(){
	struct timespec t;
	clock_gettime(0,&t);
 	return 1000000000ULL * t.tv_sec + t.tv_nsec;
}

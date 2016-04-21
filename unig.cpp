//compile with g++ -std=c++11 -O3 unig.cpp

#include <random>
#include <iostream>
#include <algorithm>
using namespace std;
constexpr uint64_t max_val = 30001;
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
	for(uint64_t i = fhd+1;i < max_val;i++){
		vals.push_back(pow(10,-2.341611959e-4 * i));
	}
	normalize(0.9,vals.begin(),vals.begin() + fhd+1);
	normalize(0.1,vals.begin()+fhd+1,vals.end());
	return discrete_distribution<uint64_t> (vals.begin(),vals.end());
}
uint64_t counts[max_val];
int main(){
    constexpr uint64_t rands = 100000000ULL;
    default_random_engine generator(1);
    discrete_distribution<uint64_t> rand_dist =  init_dist();
    for(int64_t i = 0; i < rands; i++){
        counts[rand_dist(generator)]++;
    }
    for(size_t i = 0;i < max_val;i++){
        cout << i << "\t\t" << counts[i] / double(rands) << "\n";
    }
    return 0;
}

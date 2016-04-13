#include "cache.h"
#include <string.h>
#include <time.h>
#include <cstdio>

constexpr int maxmem = 10000;

uint64_t get_time_ns(){
	struct timespec t;
	clock_gettime(0,&t);
 	return 1000000000ULL * t.tv_sec + t.tv_nsec;
}

int main(){
    cache_t c = create_cache(100000,NULL);
    char val[] = "this is a reasonably long value bufffffffffffffffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrrrrrrrrrrrrrrrrrrrrrrbufffffffffffffffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrrrrrrrrrrrrrrrrrrrrrrbufffffffffffffffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrrrrrrrrrrrrrrrrrrrrrrbufffffffffffffffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrrrrrrrrrrrrrrrrrrrrrrbufffffffffffffffffffffffffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrrrrrrrrrrrrrrrrrrrrrr";
    char key[] = "this is a reasonable key";
    cache_set(c,(key_type)key,(val_type)val,strlen(val)+1);
    printf("did something\n");
    fflush(stdout);
    uint64_t start = get_time_ns();
    uint32_t nulval = 0;
    for(int i = 0; i < 20; i++){
        val_type v = cache_get(c,(key_type)key,&nulval);
        if(v == NULL){
            printf("get failed\n");
        }
        else if(strcmp((char*)(v),val)){
            printf("get returned bad value\n");
        }
    }
    double time_passed = (get_time_ns() - start) / 1000000000.0;
    printf("time taken = %f\n",time_passed);
    destroy_cache(c);
    return 0;
}

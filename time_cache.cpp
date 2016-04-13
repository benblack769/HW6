#include "cache.h"
#include <string.h>

constexpr int maxmem = 10000;

int main(){
    cache_t cache = create_cache(maxmem,NULL);
    key_type k = (key_type)("hi there");
    val_type v = (val_type)("valueable");
    cache_set(cache,k,v,strlen(v)+1);
    uint32_t vs = 0;
    cache_get(cache,k,&vs);
    return 0;
}

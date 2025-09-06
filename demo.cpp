
#include "memory_pool.h"

struct YourObject {
    int a = 0;
    double b = 0.0;
    YourObject(int a, double b) : a(a), b(b) {} 

};

int main() {
    MemoryPool<YourObject, 1000> pool; // Where 1000 is the number of objects you want to pre-allocate

    YourObject *yo = pool.new_element( 1, 2.0 ); // Where args are passed to the constructor of YourObject
    pool.delete_element(yo); // Returns the element to the pool, and calls the destructor
    return 0;
}
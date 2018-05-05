#include "mmap-profiler/profiler.hpp"
#include <unistd.h>

#include <iostream>
using std::cout;
using std::endl;

int main() {
    ProfilerSharedObject* so = ProfilerSharedObject::Open();
    if (!so) {
        cout << "Error opening the shared object" <<endl;
        return -1;
    }

    for (int i = 0; i < 100; i++) {
        BasicLoggable* l = new BasicLoggable((void*)"0123456789", i % 10 + 1);
        if (so->Log(l) < 0) {
            cout << "Failed! " << i << endl;
            i--;
        }
        delete l;
    }
    cout << "Done" << endl;
    ProfilerSharedObject::Destroy();
    return 0;
}
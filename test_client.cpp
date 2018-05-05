#include "profiler.hpp"
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

    LOG_ITEM d;
    d.data = (void*)"0123456789";
    for (int i = 0; i < 100; i++) {
        d.size = i % 10 + 1;
        if (so->Log(d) < 0) {
            cout << "Failed! " << i << endl;
            i--;
            //sleep(1);
        }
    }
    cout << "Done" << endl;
    ProfilerSharedObject::Destroy();
    return 0;
}
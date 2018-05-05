#include "mmap-profiler/profiler.hpp"
#include "mmap-profiler/plugins/quic-plugin.hpp"
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

    for (int i = 0; i < 1000; i++) {
        QuicLoggable l(i % 10, i % 100);
        if (so->Log(&l) < 0) {
            cout << "Failed! " << i << endl;
            i--;
        }
        usleep(50000);
    }
    cout << "Done" << endl;
    ProfilerSharedObject::Destroy();
    return 0;
}
#include "mmap-profiler/profiler.hpp"
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <iostream>
using std::cout;
using std::endl;

static volatile int g_keep_running = 1;

void intHandler(int) {
    g_keep_running = 0;
}

int main() {
    ProfilerSharedObject* so = ProfilerSharedObject::Create(300);
    if (!so) {
        cout << "Error creating the shared object" <<endl;
        return -1;
    }

    signal(SIGINT, intHandler);

    while (g_keep_running) {
        std::vector<Loggable*> logs;
        so->ConsumeLogs(logs, BasicLoggable::Create);
        for (std::vector<Loggable*>::iterator it = logs.begin(); it != logs.end(); it++) {
            char buf[256] = {0};
            memcpy(buf, (*it)->GetData(), (*it)->GetSize());
            cout << buf << endl;
            delete *it;
        }
    }

    ProfilerSharedObject::Destroy();
    return 0;
}
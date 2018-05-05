#include "profiler.hpp"
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
    ProfilerSharedObject* so = ProfilerSharedObject::Create();
    if (!so) {
        cout << "Error creating the shared object" <<endl;
        return -1;
    }

    signal(SIGINT, intHandler);

    while (g_keep_running) {
        std::vector<LOG_ITEM> logs;
        so->ConsumeLogs(logs);
        for (std::vector<LOG_ITEM>::iterator it = logs.begin(); it != logs.end(); it++) {
            char buf[256] = {0};
            memcpy(buf, it->data, it->size);
            cout << buf << endl;
        }
    }

    ProfilerSharedObject::Destroy();
    return 0;
}
#include "profiler.hpp"
#include <signal.h>

static volatile int g_keep_running = 1;

void intHandler(int) {
    g_keep_running = 0;
}

int main() {
    ProfilerSharedObject* so = ProfilerSharedObject::Create();

    signal(SIGINT, intHandler);

    while (g_keep_running);

    ProfilerSharedObject::Destroy(so);
    return 0;
}
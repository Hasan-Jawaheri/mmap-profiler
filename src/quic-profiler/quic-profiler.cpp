#include "profiler.hpp"
#include "plugins/quic-plugin.hpp"
#include <signal.h>

#include <iostream>
using std::cout;
using std::endl;

static volatile int g_keep_running = 1;

class QuicProfiler_t {
public:
    QuicProfiler_t() {}
    ~QuicProfiler_t() {}

    void OnLog(QuicLoggable* log) {

    }

    void Render() {
        cout << "LOL " << endl;
    }

} QuicProfiler;

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
        std::vector<Loggable*> logs;
        so->ConsumeLogs(logs, QuicLoggable::Create);
        for (std::vector<Loggable*>::iterator it = logs.begin(); it != logs.end(); it++) {
            QuicProfiler.OnLog((QuicLoggable*)*it);
            delete *it;
        }
        if (logs.size() > 0)
            QuicProfiler.Render();
    }

    ProfilerSharedObject::Destroy();
    return 0;
}
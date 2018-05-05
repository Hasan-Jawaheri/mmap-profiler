#include "mmap-profiler/profiler.hpp"
#include "mmap-profiler/plugins/quic-plugin.hpp"
#include <signal.h>
#include <curses.h>

#include <algorithm>
#include <functional>
#include <array>
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;

#include <iostream>
using std::cout;
using std::endl;

static volatile int g_keepRunning = 1;
extern int LINES, COLUMNS;

class QuicProfiler_t {
    struct QUIC_SESSION_DATA {
        long long connId;
        long long queueSize;
    };

    unordered_map<long long, QUIC_SESSION_DATA> m_runningSessions;

public:
    QuicProfiler_t() {}
    ~QuicProfiler_t() {}

    void OnLog(QuicLoggable* log) {
        long long connId = log->m_data.connId;
        long long queueSize = log->m_data.queueSize;
        auto it = m_runningSessions.find(connId);
        if (it == m_runningSessions.end()) {
            if (queueSize > 0) {
                QUIC_SESSION_DATA qd {connId, queueSize};
                m_runningSessions.insert(std::pair<long long, QUIC_SESSION_DATA>(connId, qd));
            }
        } else {
            it->second.queueSize = queueSize;
            if (queueSize == 0)
                m_runningSessions.erase(it);
        }
    }

    void Render() {
        int sh, sw;
        char buf[1024];
        getmaxyx(stdscr, sh, sw);
        int barlen = sw - 20 - 6;
        clear();

        mvprintw(0, 0, "[  connId  ][Queue Size]");

        if (m_runningSessions.size() > 0) {
            vector<QUIC_SESSION_DATA> arr;
            
            int curline = 1;
            for (auto it = m_runningSessions.begin(); it != m_runningSessions.end(); it++) {
                arr.push_back(it->second);
            };

            std::sort(arr.begin(), arr.end(), [](QUIC_SESSION_DATA a, QUIC_SESSION_DATA b) {
                return a.queueSize > b.queueSize;
            });
            if ((int)arr.size() > sh - 1)
                arr.erase(arr.begin() + (arr.size() - (sh - 1)), arr.end());
            int longest_queue = arr[0].queueSize;

            if (longest_queue > 0) {
                for (unsigned int i = 0; i < arr.size(); i++) {
                    char bar[1024] = {0};
                    int curbarlen = (int)((float)barlen * ((float)arr[i].queueSize / (float)longest_queue));
                    for (int j = 0; j < barlen; j++) {
                        if (j < curbarlen) bar[j] = '#';
                        else bar[j] = ' ';
                    }
                    sprintf(buf, "[%10ld][%10ld][%s]", (long int)arr[i].connId % 10000000000, (long int)arr[i].queueSize, bar);
                    mvprintw(curline, 0, buf);
                    curline++;
                }
            }
        }
        
        refresh();
    }

} QuicProfiler;

void intHandler(int) {
    g_keepRunning = 0;
}

int main() {
    initscr();
    noecho();
    curs_set(false);

    ProfilerSharedObject* so = ProfilerSharedObject::Create();
    if (!so) {
        cout << "Error creating the shared object" <<endl;
        return -1;
    }

    signal(SIGINT, intHandler);

    QuicProfiler.Render();
    while (g_keepRunning) {
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
    endwin();
    return 0;
}
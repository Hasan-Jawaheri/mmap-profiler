#include "mmap-profiler/profiler.hpp"
#include "mmap-profiler/plugins/quic-plugin.hpp"
#include <signal.h>
#include <curses.h>

#include <algorithm>
#include <functional>
#include <array>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>

using std::unordered_map;
using std::vector;
using std::ofstream;
using std::ios;
using std::string;
using std::cout;
using std::endl;

static volatile int g_keepRunning = 1;
extern int LINES, COLUMNS;

class QuicProfiler_t {
    ofstream m_logFile;
    // from string 'pid:connId' to the data of that session
    unordered_map<string, QuicLoggable::DATA> m_runningSessions;

    string makeMapKey(long long pid, long long connId) {
        return std::to_string(pid) + ":" + std::to_string(connId);
    }

public:
    QuicProfiler_t() {}
    ~QuicProfiler_t() {}

    int SetLogFilename(string filename) {
        m_logFile.open(filename, ios::out);
        return m_logFile.is_open() ? 0 : 1;
    }

    void CloseLogFile() {
        m_logFile.close();
    }

    void OnLog(QuicLoggable* log, unsigned long long curTime) {
        long long connId = log->m_data.connId;
        long long queueSize = log->m_data.queueSize;
        long long pid = log->m_data.procId;
        auto it = m_runningSessions.find(makeMapKey(pid, connId));
        if (it == m_runningSessions.end()) {
            if (queueSize > 0) {
                QuicLoggable::DATA qd(connId, queueSize, pid);
                m_runningSessions.insert(std::pair<string, QuicLoggable::DATA>(makeMapKey(pid, connId), qd));
            }
        } else {
            it->second.queueSize = queueSize;
            if (queueSize == 0)
                m_runningSessions.erase(it);
        }
        m_logFile << "[" << curTime << "][" << pid << "][" << connId << "][" << queueSize << "]\n";
    }

    void Render() {
        int sh, sw;
        char buf[1024];
        getmaxyx(stdscr, sh, sw);
        int barlen = sw - 31 - 8;
        clear();

        mvprintw(0, 0, "[  connId   ][   pid    ][Queue Size]");

        if (m_runningSessions.size() > 0) {
            vector<QuicLoggable::DATA> arr;
            
            int curline = 1;
            for (auto it = m_runningSessions.begin(); it != m_runningSessions.end(); it++) {
                arr.push_back(it->second);
            };

            std::sort(arr.begin(), arr.end(), [](QuicLoggable::DATA a, QuicLoggable::DATA b) {
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
                    sprintf(buf, "[%11ld][%10ld][%10ld][%s]", (long int)arr[i].connId % 10000000000, (long int)arr[i].procId, (long int)arr[i].queueSize, bar);
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

long long getSysElapsedTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int main(int argc, char* argv[]) {
    if (argc != 2 || QuicProfiler.SetLogFilename(string(argv[1])) != 0) {
        cout << "Usage: " << argv[0] << " <output filename>" << endl;
        return -1;
    }

    initscr();
    noecho();
    curs_set(false);

    ProfilerSharedObject* so = ProfilerSharedObject::Create();
    if (!so) {
        cout << "Error creating the shared object" <<endl;
        return -2;
    }

    signal(SIGINT, intHandler);

    long long initialTime = getSysElapsedTime();

    QuicProfiler.Render();
    while (g_keepRunning) {
        std::vector<Loggable*> logs;
        so->ConsumeLogs(logs, QuicLoggable::Create);
        if (logs.size() > 0) {
            long long curTime = getSysElapsedTime() - initialTime;
            for (std::vector<Loggable*>::iterator it = logs.begin(); it != logs.end(); it++) {
                QuicProfiler.OnLog((QuicLoggable*)*it, curTime);
                delete *it;
            }
            QuicProfiler.Render();
        }
    }

    ProfilerSharedObject::Destroy();
    endwin();

    QuicProfiler.CloseLogFile();

    return 0;
}
#pragma once

#include "mmap-profiler/profiler.hpp"
#include <string.h>

class QuicLoggable : public Loggable {
public:
    struct DATA {
        DATA() {}
        DATA(long long c, long long q, long long p) : connId(c), queueSize(q), procId(p) {}
        long long connId;
        long long queueSize;
        long long procId;
    } m_data;

    QuicLoggable(void* data, int size) {
        memcpy(&m_data, data, size);
    }
    QuicLoggable(long long connId, long long queueSize, long long pid) : m_data(connId, queueSize, pid) {}
    ~QuicLoggable() {}
    
    virtual void* GetData() const { return (void*)&m_data; }
    virtual int   GetSize() const { return sizeof(DATA); }

    static Loggable* Create(void* data, int size) { return new QuicLoggable(data, size); }
};

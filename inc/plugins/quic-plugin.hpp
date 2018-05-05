#pragma once

#include "profiler.hpp"
#include <string.h>

class QuicLoggable : public Loggable {
public:
    struct DATA {
        DATA() {}
        DATA(long long c, long long q) : queueSize(q), connId(c) {}
        long long queueSize;
        long long connId;
    } m_data;

    QuicLoggable(void* data, int size) {
        memcpy(&m_data, data, size);
    }
    QuicLoggable(long long connId, long long queueSize) : m_data(connId, queueSize) {}
    ~QuicLoggable() {}
    
    virtual void* GetData() const { return (void*)&m_data; }
    virtual int   GetSize() const { return sizeof(DATA); }

    static Loggable* Create(void* data, int size) { return new QuicLoggable(data, size); }
};

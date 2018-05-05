#pragma once

#include "profiler.hpp"

class QuicLoggable : public Loggable {
    void* m_data;
    int   m_size;

public:
    QuicLoggable(void* data, int size) : m_data(data), m_size(size) {}
    ~QuicLoggable() {}
    
    virtual void* GetData() const { return m_data; }
    virtual int   GetSize() const { return m_size; }

    static Loggable* Create(void* data, int size) { return new QuicLoggable(data, size); }
};

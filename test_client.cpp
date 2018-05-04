#include "profiler.hpp"

#include <iostream>
using std::cout;
using std::endl;

int main() {
    ProfilerSharedObject* so = ProfilerSharedObject::Open();
    cout << "Max size: " << so->m_shared_mem_size << " " << so->m_queue_start << endl;
    ProfilerSharedObject::Destroy(so);
    return 0;
}
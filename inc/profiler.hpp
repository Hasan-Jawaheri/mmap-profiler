#pragma once

#include <cstddef>
#include <pthread.h>

#include <vector>

#define DEFAULT_SHARED_MAP_SIZE (sizeof(ProfilerSharedObject)+50) // 200*1024*1024

struct LOG_ITEM {
    void* data;
    int size;
};

class ProfilerSharedObject {
    /**
     * These are not shared
     */
    
    /** true iff this process called ::Create() */
    static bool m_is_master;
    /** beginning of the mmap'd shared memory */
    static char* m_shared_mem_start;
    /** Address of the first byte after the ProfilerSharedObject in m_shared_mem_size */
    static char* m_queue_mem_start;
    /** Address of the byte after the last byte in the shared memory */
    static char* m_queue_mem_end;
    /** Memory allocated for logs consumed, freed by the library */
    static char* m_allocated_consumption_mem;
    /** Size of m_allocated_consumption_mem */
    static size_t m_allocated_consumption_mem_size;

    /**
     * Shared variables
     */

    /** size of the mmap'd shared memory */
    volatile size_t m_shared_mem_size;
    /** size of the queue memory */
    volatile long long m_queue_mem_size;
    /** First and last byte offsets that are allocated in the queue */
    volatile long long m_queue_start, m_queue_end;
    /** Process-shared mutex used (by all processes) to allocate/free queue memory */
    pthread_mutex_t m_queue_mutex;
    /** Process-shared conditional variable on m_queue_mutex */
    pthread_cond_t m_queue_cond;

    /**
     * Adds data at the end of the queue
     */
    int WriteToQueue(void* user_mem, size_t size);

public:
    /**
     * Not-shared stuff
     */

    /**
     * Create the shared process data and mmap it to this process.
     * The newly mmap'd memory has the following layout:
     * - The size of the memory is in m_shared_mem_size
     * - The memory starts at m_shared_mem_start
     * - The first (sizeof(ProfilerSharedObject)) bytes of the memory (starting at
     *   m_shared_mem_start) is a (shared) instance of ProfilerSharedObject.
     * - The rest of the bytes of the shared memory is used for the queue
     * 
     * @param max_size  The maximum size of the shared memory object (must be at least sizeof(ProfilerSharedObject))
     * @return          The shared ProfilerSharedObject
     */
    static ProfilerSharedObject* Create(size_t max_size=DEFAULT_SHARED_MAP_SIZE);

    /**
     * Opens an already created shared object and returns the ProfilerSharedObject
     * object at the beginning of it.
     * 
     * @return  The shared ProfilerSharedObject
     */
    static ProfilerSharedObject* Open();

    /**
     * @return The shared ProfilerSharedObject if Create() or Open() has been called. NULL otherwise.
     */
    static ProfilerSharedObject* Get();

    /**
     * Destroys the memory mapped shared data for this process. If this process is
     * the creator (called ::Create) then it will also free the shared object from the OS.
     */
    static void Destroy();

    /**
     * Shared stuff
     */

    ProfilerSharedObject(size_t max_size);

    int Log(LOG_ITEM it);
    /** LOGS ARE ERASED ON NEXT CALL */
    void ConsumeLogs(std::vector<LOG_ITEM>& logs);
};

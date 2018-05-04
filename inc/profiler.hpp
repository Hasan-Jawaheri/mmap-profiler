#pragma once

#include <cstddef>

#define DEFAULT_SHARED_MAP_SIZE 200*1024*1024

class ProfilerSharedObject {
    /**
     * These are not shared
     */
    
    /** true iff this process called ::Create() */
    static bool m_is_master;
    /** beginning of the mmap'd shared memory */
    static void* m_shared_mem_start;
    
public:
    /** size of the mmap'd shared memory */
    size_t m_shared_mem_size;
    /** Address of the first byte after the ProfilerSharedObject in m_shared_mem_size */
    void* m_queue_start;

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
     * max_size  The maximum size of the shared memory object (must be at least sizeof(ProfilerSharedObject))
     * returns   The shared ProfilerSharedObject
     */
    static ProfilerSharedObject* Create(size_t max_size=DEFAULT_SHARED_MAP_SIZE);

    /**
     * Opens an already created shared object and returns the ProfilerSharedObject
     * object at the beginning of it.
     * 
     * returns   The shared ProfilerSharedObject
     */
    static ProfilerSharedObject* Open();

    /**
     * Destroys the memory mapped shared data for this process. If this process is
     * the creator (called ::Create) then it will also free the shared object from the OS.
     * so  The shared ProfilerSharedObject returned by Create() or Open()
     */
    static void Destroy(ProfilerSharedObject* so);

    /**
     * Shared stuff
     */

    ProfilerSharedObject(int max_size);
};

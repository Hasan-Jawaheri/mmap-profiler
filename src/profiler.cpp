#include "profiler.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define SHAREDMEM_FILENAME "/quic-profiler"

bool ProfilerSharedObject::m_is_master = false;
char* ProfilerSharedObject::m_shared_mem_start = NULL;
char* ProfilerSharedObject::m_queue_mem_start = NULL;
char* ProfilerSharedObject::m_queue_mem_end = NULL;
char* ProfilerSharedObject::m_allocated_consumption_mem = NULL;
size_t ProfilerSharedObject::m_allocated_consumption_mem_size = 0;

ProfilerSharedObject* ProfilerSharedObject::Create(size_t max_size) {
    m_is_master = true;

    if (max_size < sizeof(ProfilerSharedObject))
        max_size = sizeof(ProfilerSharedObject);
    // align for the queue size to be a multiple of 4
    max_size -= sizeof(ProfilerSharedObject);
    max_size = 4 * ((max_size+3)/4);
    max_size += sizeof(ProfilerSharedObject);
        
    Destroy();

    /**
     * Create shared memory object
     */
    int shm_fd = shm_open(SHAREDMEM_FILENAME, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    if (shm_fd <= 0)
        return NULL;
    ftruncate(shm_fd, max_size);

    /**
     * Map the object to local process memory
     */
    m_shared_mem_start = (char*)mmap(0, max_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (!m_shared_mem_start) {
        shm_unlink(SHAREDMEM_FILENAME);
        return NULL;
    }

    /**
     * Initialize the shared object data so all processes can use it
     */
    ProfilerSharedObject* so = (ProfilerSharedObject*)m_shared_mem_start;
    *so = ProfilerSharedObject(max_size);
    m_queue_mem_start = m_shared_mem_start + sizeof(ProfilerSharedObject);
    m_queue_mem_end = m_shared_mem_start + max_size;

    /**
     * Allocate memory for log consumption
     */
    m_allocated_consumption_mem = (char*)malloc(256);
    m_allocated_consumption_mem_size = 256;

    return so;
}

ProfilerSharedObject* ProfilerSharedObject::Open() {
    Destroy();

    /**
     * Open the shared object
     */
    int shm_fd = shm_open(SHAREDMEM_FILENAME, O_RDWR, S_IRUSR|S_IWUSR);
    if (shm_fd <= 0)
        return NULL;

    /**
     * map the memory to this process's local space. First map only sizeof(ProfilerSharedObject)
     * to read the m_shared_mem_size from it then map the entire region.
     */
    m_shared_mem_start = (char*)mmap(0, sizeof(ProfilerSharedObject), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (!m_shared_mem_start) {
        close(shm_fd);
        return NULL;
    }
    size_t max_size = ((ProfilerSharedObject*)m_shared_mem_start)->m_shared_mem_size;
    munmap(m_shared_mem_start, sizeof(ProfilerSharedObject));
    m_shared_mem_start = (char*)mmap(0, max_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    m_queue_mem_start = m_shared_mem_start + sizeof(ProfilerSharedObject);
    m_queue_mem_end = m_shared_mem_start + max_size;

    if (!m_shared_mem_start) {
        return NULL;
    }

    return (ProfilerSharedObject*)m_shared_mem_start;
}

ProfilerSharedObject* ProfilerSharedObject::Get() {
    return (ProfilerSharedObject*)m_shared_mem_start;
}

void ProfilerSharedObject::Destroy() {
    if (m_allocated_consumption_mem) {
        free(m_allocated_consumption_mem);
        m_allocated_consumption_mem = NULL;
    }

    if (m_shared_mem_start) {
        munmap(m_shared_mem_start, Get()->m_shared_mem_size);
        m_shared_mem_start = NULL;
    }

    if (m_is_master) {
        shm_unlink(SHAREDMEM_FILENAME);
    }
}

ProfilerSharedObject::ProfilerSharedObject(size_t max_size) : m_shared_mem_size(max_size) {
    m_queue_mem_size = max_size - sizeof(ProfilerSharedObject);
    m_queue_start = 0;
    m_queue_end = 0;

    // make the mutex process-shared
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&m_queue_mutex, &mutex_attr);
}

int ProfilerSharedObject::Log(Loggable* it) {
    void* user_mem = it->GetData();
    size_t size = it->GetSize();
    int user_mem_size = size;

    size += 4; // 4 bytes to store the allocation size
    size = 4 * ((size+3)/4); // align to multiple of 4

    long long allocated_queue_offset = -1;
    long long new_queue_end = 0; // don't immediately change m_queue_end, instead store here and change later
    
    pthread_mutex_lock(&m_queue_mutex);
    size_t available_memory = 0;
    if (m_queue_end >= m_queue_start)
        available_memory = (m_queue_mem_size - m_queue_end) + m_queue_start;
    else
        available_memory = m_queue_start - m_queue_end;
    if (available_memory > size) { // we have available memory
        allocated_queue_offset = m_queue_end;
        new_queue_end = (m_queue_end + size) % m_queue_mem_size;
    }

    if (allocated_queue_offset < 0) {
        pthread_mutex_unlock(&m_queue_mutex);
        return -1;
    }

    // write the size at the first 4 bytes (allocations and queue mem size are 4 byte aligned so this will always be safe)
    char* queue_mem = m_queue_mem_start + allocated_queue_offset;
    assert(m_queue_mem_end - queue_mem >= 4);
    *((int*)queue_mem) = user_mem_size;
    queue_mem += 4;
    allocated_queue_offset += 4;
    size -= 4;

    int64_t overflow = size - (m_queue_mem_size - allocated_queue_offset);
    if (overflow > 0) {
        memcpy(queue_mem, user_mem, size - overflow);
        memcpy(m_queue_mem_start, (char*)user_mem + size - overflow, overflow);
    } else
        memcpy(queue_mem, user_mem, size);
    
    // do this only after memcpy so consume will only consume after data has been written
    m_queue_end = new_queue_end;
        
    pthread_mutex_unlock(&m_queue_mutex);
    
    return 0;
}

void ProfilerSharedObject::ConsumeLogs(std::vector<Loggable*>& logs, Loggable* (*loggable_factory) (void* data, int size)) {
    long long queue_end = m_queue_end;
    long long queue_start = m_queue_start;
    if (queue_end != queue_start) { // not empty
        long long size_to_read = 0;
        if (queue_end > queue_start)
            size_to_read = queue_end - queue_start;
        else if (queue_end < queue_start)
            size_to_read = (m_queue_mem_size - queue_start) + queue_end;
        
        // copy the queue data, resize the buffer as necessary
        while (m_allocated_consumption_mem_size < (size_t)size_to_read) {
            m_allocated_consumption_mem = (char*)realloc(m_allocated_consumption_mem, m_allocated_consumption_mem_size*2);
            m_allocated_consumption_mem_size *= 2;
        }
        int64_t overflow = size_to_read - (m_queue_mem_size - queue_start);
        if (overflow > 0) {
            memcpy(m_allocated_consumption_mem, m_queue_mem_start + queue_start, size_to_read - overflow);
            memcpy(m_allocated_consumption_mem + size_to_read - overflow, m_queue_mem_start, overflow);
        } else
            memcpy(m_allocated_consumption_mem, m_queue_mem_start + queue_start, size_to_read);
        // free queue memory
        m_queue_start = (queue_start + size_to_read) % m_queue_mem_size;

        char* cur_mem = m_allocated_consumption_mem;
        while (size_to_read) {
            int size = *(int*)cur_mem;
            logs.push_back(loggable_factory(cur_mem + 4, size));

            size = 4 * ((size+3)/4) + 4; // include the 4 bytes of the size and align to 4
            cur_mem += size;
            size_to_read -= size;
        }
    }
}

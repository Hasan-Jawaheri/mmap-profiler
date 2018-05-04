#include "profiler.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define SHAREDMEM_FILENAME "/quic-profiler"

bool ProfilerSharedObject::m_is_master = false;
void* ProfilerSharedObject::m_shared_mem_start = NULL;

ProfilerSharedObject* ProfilerSharedObject::Create(size_t max_size) {
    if (max_size < sizeof(ProfilerSharedObject))
        return NULL;

    /**
     * Create shared memory object
     */
    m_is_master = true;
    int shm_fd = shm_open(SHAREDMEM_FILENAME, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    if (shm_fd <= 0)
        return NULL;
    ftruncate(shm_fd, max_size);

    /**
     * Map the object to local process memory
     */
    m_shared_mem_start = mmap(0, max_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
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

    return so;
}

ProfilerSharedObject* ProfilerSharedObject::Open() {
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
    m_shared_mem_start = mmap(0, sizeof(ProfilerSharedObject), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (!m_shared_mem_start) {
        close(shm_fd);
        return NULL;
    }
    size_t max_size = ((ProfilerSharedObject*)m_shared_mem_start)->m_shared_mem_size;
    munmap(m_shared_mem_start, sizeof(ProfilerSharedObject));
    m_shared_mem_start = mmap(0, max_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (!m_shared_mem_start) {
        return NULL;
    }

    return (ProfilerSharedObject*)m_shared_mem_start;
}

void ProfilerSharedObject::Destroy(ProfilerSharedObject* so) {
    if (m_is_master) {
        shm_unlink(SHAREDMEM_FILENAME);
    }
    if (m_shared_mem_start) {
        munmap(m_shared_mem_start, so->m_shared_mem_size);
        m_shared_mem_start = NULL;
    }
}

ProfilerSharedObject::ProfilerSharedObject(int max_size) : m_shared_mem_size(max_size) {
    m_queue_start = (char*)this + sizeof(ProfilerSharedObject);
}

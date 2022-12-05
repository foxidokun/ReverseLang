#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <sys/types.h>

#include "common.h"
#include "file.h"

// -------------------------------------------------------------------------------------------------

#define ERR_CASE(cond)       \
{                            \
    if (cond)                \
    {                        \
        return {nullptr, 0}; \
    }                        \
}

const file_t open_ro_file (const char* name)
{
    assert (name != nullptr && "invalid pointer");

    int fd = open (name, O_RDONLY);
    ERR_CASE (fd < 0);

    ssize_t size = file_size (fd);
    ERR_CASE (size < 0);

    void *mapped_memory = mmap(0, (size_t) size, PROT_READ, MAP_SHARED, fd, 0);
    ERR_CASE (mapped_memory == MAP_FAILED);

    return {(const char *) mapped_memory, (size_t) size};
}

#undef ERR_CASE

// -------------------------------------------------------------------------------------------------

void unmap_ro_file (file_t file)
{
    munmap (const_cast<char *>(file.content), file.size);
} 

// -------------------------------------------------------------------------------------------------

ssize_t file_size (int fd)
{
    assert (fd >= 0 && "invalid fd");

    struct stat fstats = {};

    int ret = fstat (fd, &fstats);
    if (ret == -1) { return ERROR; }

    return fstats.st_size;
}

ssize_t file_size (FILE *stream)
{
    assert (stream != NULL && "pointer can't be NULL");

    int fd = fileno (stream);
    if (fd == -1) { return ERROR; }

    
    return file_size (fd);
}
#ifndef FILE_H
#define FILE_H

#include <stdlib.h>
#include <stdio.h>

struct file_t {
    const char *content;
    size_t size;
};

const file_t open_ro_file (const char* name);

void unmap_ro_file (file_t file);

ssize_t file_size (int fd);
ssize_t file_size (FILE *stream);

#endif
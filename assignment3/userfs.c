#include "userfs.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

enum {
    BLOCK_SIZE = 1024,
    MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
    /** Block memory. */
    char *memory;
    /** How many bytes are occupied. */
    int occupied;
    /** Next block in the file. */
    struct block *next;
    /** Previous block in the file. */
    struct block *prev;

    /* PUT HERE OTHER MEMBERS */
};

struct file {
    /** Double-linked list of file blocks. */
    struct block *block_list;
    /**
     * Last block in the list above for fast access to the end
     * of file.
     */
    struct block *last_block;
    /** How many file descriptors are opened on the file. */
    int refs;
    /** File name. */
    const char *name;
    /** Files are stored in a double-linked list. */
    struct file *next;
    struct file *prev;

    /* PUT HERE OTHER MEMBERS */
    int cnt_blocks;
    bool to_delete;
};

/** List of all files. */
static struct file *file_list = NULL;
int file_number = 0;

struct filedesc {
    struct file *file;

    /* PUT HERE OTHER MEMBERS */
    struct block *block_ptr;
    int block_phase;
    int block_id;

    bool can_read;
    bool can_write;
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
// static int file_descriptor_count = 0; unused
static int file_descriptor_capacity = 0;

int min(int a, int b) {
    if (a <= b)
        return a;
    else
        return b;
}

int max(int a, int b) {
    if (a >= b)
        return a;
    else
        return b;
}

int get_descriptor(struct file *file_ptr, int flags) {
    for (int i = 0; i < file_descriptor_capacity; i++) {
        if (!file_descriptors[i]) {
            file_descriptors[i] = (struct filedesc*)malloc(sizeof(struct filedesc));
            file_descriptors[i]->file = file_ptr;
            file_descriptors[i]->block_ptr = file_ptr->block_list;
            file_descriptors[i]->block_phase = 0;
            file_descriptors[i]->block_id = 0;
            if (flags & (UFS_READ_ONLY | UFS_READ_WRITE))
                file_descriptors[i]->can_read = true;
            else
                file_descriptors[i]->can_read = false;
            if (flags & (UFS_WRITE_ONLY | UFS_READ_WRITE))
                file_descriptors[i]->can_write = true;
            else
                file_descriptors[i]->can_write = false;
            return i;
        }
    }

    int new_file_desctiptor_capacity = max(1, file_descriptor_capacity * 2);
    struct filedesc **new_file_descriptors = (struct filedesc**)malloc(new_file_desctiptor_capacity * sizeof(struct filedesc*));
    memset(new_file_descriptors, 0, new_file_desctiptor_capacity * sizeof(struct filedesc*));
    memcpy(new_file_descriptors, file_descriptors, file_descriptor_capacity * sizeof(struct filedesc*));
    free(file_descriptors);
    file_descriptors = new_file_descriptors;
    file_descriptor_capacity = new_file_desctiptor_capacity;

    for (int i = 0; i < file_descriptor_capacity; i++) {
        if (!file_descriptors[i]) {
            file_descriptors[i] = (struct filedesc*)malloc(sizeof(struct filedesc));
            file_descriptors[i]->file = file_ptr;
            file_descriptors[i]->block_ptr = file_ptr->block_list;
            file_descriptors[i]->block_phase = 0;
            file_descriptors[i]->block_id = 0;
            if (flags & (UFS_READ_ONLY | UFS_READ_WRITE))
                file_descriptors[i]->can_read = true;
            else
                file_descriptors[i]->can_read = false;
            if (flags & (UFS_WRITE_ONLY | UFS_READ_WRITE))
                file_descriptors[i]->can_write = true;
            else
                file_descriptors[i]->can_write = false;
            return i;
        }
    }
    return -1;
}

void delete_file(struct file *file_ptr) {
    struct block *cur_block = file_ptr->block_list;
    while (cur_block->next) {
        struct block *next_block = cur_block->next;
        free(cur_block->memory);
        free(cur_block);
        cur_block = next_block;
    }
    free(cur_block->memory);
    free(cur_block);

    if (file_ptr->next == NULL && file_ptr->prev == NULL) {
        file_list = NULL;
    }
    else if (file_ptr->prev == NULL) {
        file_list = file_ptr->next;
        file_ptr->next->prev = NULL;
    }
    else if (file_ptr->next == NULL) {
        file_ptr->prev->next = NULL;
    }
    else {
        file_ptr->prev->next = file_ptr->next;
        file_ptr->next->prev = file_ptr->prev;
    }
    free((char*)file_ptr->name);
    free(file_ptr);
}

enum ufs_error_code
ufs_errno()
{
    return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
    if (!(flags & (UFS_READ_ONLY | UFS_WRITE_ONLY | UFS_READ_WRITE))) {
        flags |= UFS_READ_WRITE;
    }

    if (file_list) {
        struct file *file_ptr = file_list;
        while (file_ptr) {
            if (strcmp(file_ptr->name, filename) == 0 && !file_ptr->to_delete) {
                file_ptr->refs++;
                return get_descriptor(file_ptr, flags);
            }
            file_ptr = file_ptr->next;
        }
    }

    if (flags & UFS_CREATE) {
        struct file *new_file;
        if (!file_list) {
            file_list = (struct file*)malloc(sizeof(struct file));
            memset(file_list, 0, sizeof(struct file));
            new_file = file_list;
        }
        else {
            struct file *file_ptr = file_list;
            while (file_ptr->next) {
                file_ptr = file_ptr->next;
            }
            new_file = (struct file*)malloc(sizeof(struct file));
            memset(new_file, 0, sizeof(struct file));
            new_file->prev = file_ptr;
            file_ptr->next = new_file;
        }
        new_file->name = strdup(filename);
        new_file->refs++;
        new_file->block_list = (struct block*)malloc(sizeof(struct block));
        new_file->block_list->memory = (char*)malloc(BLOCK_SIZE);
        new_file->block_list->occupied = 0;
        new_file->block_list->next = NULL;
        new_file->block_list->prev = NULL;
        new_file->last_block = new_file->block_list;
        new_file->cnt_blocks = 1;
        new_file->to_delete = false;
        return get_descriptor(new_file, flags);
    }
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
    if (fd >= 0 && fd < file_descriptor_capacity && file_descriptors[fd]) {
        if (!file_descriptors[fd]->can_write) {
            ufs_error_code = UFS_ERR_NO_PERMISSION;
            return -1;
        }

        for (int i = 0; i < size; i++) {
            if (file_descriptors[fd]->file->cnt_blocks * BLOCK_SIZE > MAX_FILE_SIZE) {
                ufs_error_code = UFS_ERR_NO_MEM;
                return -1;
            }
            char c = buf[i];
            struct block *cur_block = file_descriptors[fd]->block_ptr;
            cur_block->memory[file_descriptors[fd]->block_phase] = c;
            file_descriptors[fd]->block_phase++;
            cur_block->occupied = max(cur_block->occupied, file_descriptors[fd]->block_phase);
            if (file_descriptors[fd]->block_phase == BLOCK_SIZE) {
                file_descriptors[fd]->file->cnt_blocks++;
                struct block *new_block = (struct block*)malloc(sizeof(struct block));
                cur_block->next = new_block;
                new_block->prev = cur_block;
                new_block->next = NULL;
                new_block->occupied = 0;
                new_block->memory = (char*)malloc(BLOCK_SIZE);
                file_descriptors[fd]->file->last_block = new_block;
                file_descriptors[fd]->block_ptr = new_block;
                file_descriptors[fd]->block_phase = 0;
                file_descriptors[fd]->block_id++;
            }
        }
        return size;
    }
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
    if (fd >= 0 && fd < file_descriptor_capacity && file_descriptors[fd]) {
        if (!file_descriptors[fd]->can_read) {
            ufs_error_code = UFS_ERR_NO_PERMISSION;
            return -1;
        }

        for (int i = 0; i < size; i++) {
            struct block *cur_block = file_descriptors[fd]->block_ptr;
            if (cur_block->occupied <= file_descriptors[fd]->block_phase) {
                return i;
            }
            buf[i] = cur_block->memory[file_descriptors[fd]->block_phase];
            file_descriptors[fd]->block_phase++;
            if (file_descriptors[fd]->block_phase == BLOCK_SIZE) {
                file_descriptors[fd]->block_ptr = cur_block->next;
                file_descriptors[fd]->block_phase = 0;
                file_descriptors[fd]->block_id++;
            }
        }
        return size;
    }
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}

int
ufs_close(int fd)
{
    if (fd >= 0 && fd <= file_descriptor_capacity && file_descriptors[fd]) {
        file_descriptors[fd]->file->refs--;
        if (file_descriptors[fd]->file->refs == 0 && file_descriptors[fd]->file->to_delete) {
            delete_file(file_descriptors[fd]->file);
        }
        free(file_descriptors[fd]);
        file_descriptors[fd] = NULL;
        return 0;
    }
    else{
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
}

int
ufs_delete(const char *filename)
{
    if (file_list) {
        struct file *file_ptr = file_list;
        while (file_ptr) {
            if (strcmp(file_ptr->name, filename) == 0) {
                if (file_ptr->refs == 0) {
                    delete_file(file_ptr);
                }
                else {
                    file_ptr->to_delete = true;
                }
                return 0;
            }
            file_ptr = file_ptr->next;
        }
    }

    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}

int
ufs_resize(int fd, size_t new_size) {
    if (fd >= 0 && fd <= file_descriptor_capacity && file_descriptors[fd]) {
        int last_block = new_size / BLOCK_SIZE;
        int last_phase = new_size % BLOCK_SIZE;

        for (int i = 0; i < file_descriptor_capacity; i++) {
            if (file_descriptors[i] && file_descriptors[fd]->file == file_descriptors[i]->file) {
                while (file_descriptors[i]->block_id > last_block) {
                    file_descriptors[i]->block_ptr = file_descriptors[i]->block_ptr->prev;
                    file_descriptors[i]->block_phase = BLOCK_SIZE - 1;
                    file_descriptors[i]->block_id--;
                }
                if (file_descriptors[i]->block_id == last_block) {
                    file_descriptors[i]->block_phase = min(file_descriptors[i]->block_phase, last_phase);
                }
            }
        }

        struct file *cur_file = file_descriptors[fd]->file;

        while (cur_file->cnt_blocks > last_block + 1) {
            struct block *tmp_block = cur_file->last_block;
            tmp_block->prev->next = NULL;
            cur_file->last_block = tmp_block->prev;
            free(tmp_block->memory);
            free(tmp_block);
            cur_file->cnt_blocks--;
        }
        while (cur_file->cnt_blocks < last_block + 1) {
            struct block *tmp_block = (struct block*)malloc(sizeof(struct block));
            tmp_block->memory = (char*)malloc(BLOCK_SIZE);
            cur_file->last_block->occupied = BLOCK_SIZE;
            cur_file->last_block->next = tmp_block;
            tmp_block->prev = cur_file->last_block;
            tmp_block->next = NULL;
            tmp_block->occupied = BLOCK_SIZE;
            cur_file->last_block = tmp_block;
            cur_file->cnt_blocks++;
        }
        cur_file->last_block->occupied = last_phase;

        return 0;
    }
    else{
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
}
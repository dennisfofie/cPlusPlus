#include <iostream>
#include <cstddef>
#include <pthread.h>
#include <unistd.h>
#include <cstring>

using namespace std;

typedef char ALIGN[16];

union header {
    struct header_t {
        size_t size;
        unsigned is_free;
        struct header_t *next;
    } s;

    ALIGN stub;
};

typedef union header header_t;

header_t *head, *tail;
pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size) {
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free && curr->s.size >= size) {
            return curr;
        }
        curr = (header_t*)curr->s.next;
    }
    return NULL;
}

void *malloc(size_t size) {
    size_t total_size;
    void* block;
    header_t *header;

    if (!size) {
        return NULL;
    }

    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);

    if (header) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }

    total_size = sizeof(header_t) + size;

    block = sbrk(total_size);

    if (block == (void*) - 1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = (header_t*)block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if (!head) {
        head = header;
    }

    if (tail) {
        tail = header;
    }

    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
}

void free(void *block) {
    header_t  *header, *tmp;
    void *programbreak;

    if (!block) {
        return;
    }

    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*) block - 1;
    programbreak = sbrk(0);

    if ((char*)block + header->s.size == programbreak) {
        if (head == tail) {
            head = tail = NULL;
        }
        else {
            tmp = head;
            while (tmp) {
                if ((header_t*)tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = (header_t*)tmp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void *calloc(size_t num, size_t nsize) {
    size_t  size;
    void* block;

    if (!num || !nsize) {
        return NULL;
    }

    size = num * nsize;

    if (nsize != size / num) {
        return NULL;
    }

    block = malloc(size);

    if (!block) {
        return NULL;
    }

    memset(block, 0, size);

    return block;
}

void *realloc(void* block, size_t size) {
    header_t *header;
    void * ret;

    if (!block || !size) {
        return malloc(size);
    }

    header = (header_t*)block - 1;

    if (header->s.size >= size) {
        return block;
    }

    ret = malloc(size);

    if (ret) {
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}

int main() {
    size_t size = 20;
    cout << "size address" << &size << endl;
    

    header_t *ptr = get_free_block(size);
    cout << "Free memory block " << ptr << endl;

    void *block = malloc(size);

    if (block == NULL) {
        cout << "No memory is allocated" << endl;
    }
    else {
        cout << "allocated memory " << block << endl;

    }

    free(block);



    return 0;
}
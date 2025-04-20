#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_BUFFER_SIZE 101

typedef struct {
    ChunkMessage buffer[MAX_BUFFER_SIZE];
    int in;
    int out;
    int count;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} ChunkBuffer;



ChunkBuffer shared_buffer;

void init_chunk_buffer() {
    shared_buffer.in = 0;
    shared_buffer.out = 0;
    shared_buffer.count = 0;
    pthread_mutex_init(&shared_buffer.mutex, NULL);
    sem_init(&shared_buffer.empty, 0, MAX_BUFFER_SIZE);
    sem_init(&shared_buffer.full, 0, 0);
}

void run_peer(int peer_id){
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "peer_pipe_%d", peer_id);

    int fd = open(fifo_name, O_RDONLY);
    if (fd < 0) {
        perror("open fifo (peer)");
        exit(1);
    }

    printf("[PEER %d] Started and waiting for chunks...\n", peer_id);

    ChunkMessage msg;
    init_chunk_buffer();
    while (read(fd, &msg, sizeof(msg)) > 0) {
        printf("[PEER %d] Received Chunk %d | Offset: %d | Size: %d bytes\n",
               peer_id, msg.chunk_id, msg.start_offset, msg.size);
               
               
            }
            produce_chunk(msg);

    close(fd);
}

void produce_chunk(ChunkMessage chunk) {
    sem_wait(&shared_buffer.empty);
    pthread_mutex_lock(&shared_buffer.mutex);

    shared_buffer.buffer[shared_buffer.in] = chunk;
    shared_buffer.in = (shared_buffer.in + 1) % MAX_BUFFER_SIZE;
    shared_buffer.count++;

    printf("Produced chunk %d (buffer count: %d)\n", chunk.chunk_id, shared_buffer.count);

    pthread_mutex_unlock(&shared_buffer.mutex);
    sem_post(&shared_buffer.full);
}

ChunkMessage consume_chunk() {
    sem_wait(&shared_buffer.full);
    pthread_mutex_lock(&shared_buffer.mutex);

    ChunkMessage chunk = shared_buffer.buffer[shared_buffer.out];
    shared_buffer.out = (shared_buffer.out + 1) % MAX_BUFFER_SIZE;
    shared_buffer.count--;

    printf("Consumed chunk %d (buffer count: %d)\n", chunk.chunk_id, shared_buffer.count);

    pthread_mutex_unlock(&shared_buffer.mutex);
    sem_post(&shared_buffer.empty);

    return chunk;
}

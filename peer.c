#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_BUFFER_SIZE 101

typedef struct {
    ChunkMessage buffer[MAX_BUFFER_SIZE];
    int in;
    int out;
    int count; // size count
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



void *upload_thread_func(void *arg){

}

void* download_thread_func(void *arg){

}

void run_peer(int peer_id){
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "peer_pipe_%d", peer_id);

    int fd = open(fifo_name, O_RDONLY | O_NONBLOCK);
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
            //produce_chunk(msg);
            pthread_t upload_thread, download_thread;

        if (pthread_create(&upload_thread, NULL, upload_thread_func, (void*)(long)peer_id) != 0) {
            perror("pthread_create upload");
            exit(1);
        }
    
        if (pthread_create(&download_thread, NULL, download_thread_func, (void*)(long)peer_id) != 0) {
            perror("pthread_create download");
            exit(1);
        }
        close(fd);

    
}

int compare_chunks(const void *a, const void *b) {
    ChunkMessage *chunk_a = (ChunkMessage *)a;
    ChunkMessage *chunk_b = (ChunkMessage *)b;
    
    return chunk_a->chunk_id - chunk_b->chunk_id;
}

void work_done(int peer_id){
    char file_name[32];
    sprintf(file_name, "peer_%d_downloaded_file", peer_id);
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("Error opening shared memory");
        exit(1);
    }

    // Map the shared memory to the process's address space
    status_ptr = (SharedStatus *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (status_ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(1);
    }
    // Open the file for writing the downloaded chunks
    int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("Error opening file for writing");
        exit(1);
    }
    ChunkMessage *temp_buff = malloc(sizeof(ChunkMessage) * shared_buffer.count);
    for (int i = 0; i < shared_buffer.count; i++) {
        temp_buff[i] = shared_buffer.buffer[i];
    }
    

    qsort(temp_buff, shared_buffer.count, sizeof(ChunkMessage), compare_chunks);
    // Write each downloaded chunk to the file
    for (int i = 0; i < shared_buffer.count; i++) {
        {
            ssize_t bytes_written = write(fd, shared_buffer.buffer[i].data, shared_buffer.buffer[i].size);
            if (bytes_written == -1) {
                perror("Error writing to file");
                close(fd);
                exit(1);
            }
            printf("[PEER %d] completed it's file...\n", peer_id);
        }
    }

    // Close the file after writing
    close(fd);

    // Now, update the shared memory to indicate the peer has completed downloading
    status_ptr->peer_done[peer_id] = 1;  // 1 means completed
    printf("[PEER %d] Peer has completed downloading all chunks!\n", peer_id);
    close(shm_fd);
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

// ChunkMessage consume_chunk() {
//     sem_wait(&shared_buffer.full);
//     pthread_mutex_lock(&shared_buffer.mutex);

//     ChunkMessage chunk = shared_buffer.buffer[shared_buffer.out];
//     shared_buffer.out = (shared_buffer.out + 1) % MAX_BUFFER_SIZE;
//     shared_buffer.count--;

//     printf("Consumed chunk %d (buffer count: %d)\n", chunk.chunk_id, shared_buffer.count);

//     pthread_mutex_unlock(&shared_buffer.mutex);
//     sem_post(&shared_buffer.empty);

//     return chunk;
// }

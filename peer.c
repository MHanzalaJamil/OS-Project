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

pthread_mutex_t mutex_lock_for_buffer = PTHREAD_MUTEX_INITIALIZER;
total_downloaded = 0;
total_chunks = 0;
//sem_t reader, writer;
ChunkMessage buff[MAX_CHUNKS];


void *upload_thread_func(void *arg){
    int peer_id = *(int*)arg;
    char pipe_name[32];
    sprintf(pipe_name, "peer_pipe_%d", peer_id);

    printf("[Peer %d] Upload thread started.\n", peer_id);
    
    int fd = open(pipe_name, O_WRONLY);
    //sem_wait(&writer);
    pthread_mutex_lock(&mutex_lock_for_buffer);

    if(write(fd, &buff[0], sizeof(buff[0])) == -1){
        perror("Error Pipeline");
        pthread_exit(1);
    }

    pthread_mutex_unlock(&mutex_lock_for_buffer);
    //sem_post(&writer);
    close(fd);
    return NULL;
}

void* download_thread_func(void *arg) {
    int peer_id = *(int*) arg;

    char pipe_name[32];
    int i = 0;
    while(total_downloaded != total_chunks){

        i++;

        if(i == peer_id) continue;

        sprintf(pipe_name, "peer_pipe_%d", i);
        int fd = open(pipe_name, O_RDONLY);
        pthread_mutex_lock(&mutex_lock_for_buffer);
        ChunkMessage msg;
        if(read(fd, &msg, sizeof(msg)) == -1){
            perror("Error Pipeline");
            pthread_exit(1);
        }
        write_to_my_buff(msg);
        pthread_mutex_unlock(&mutex_lock_for_buffer);
        close(fd);
        
    }
    return NULL;
}


void write_to_my_buff(ChunkMessage msg){
    buff[total_downloaded++] = msg;
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

    while (read(fd, &msg, sizeof(msg)) > 0) {
        printf("[PEER %d] Received Chunk %d | Offset: %d | Size: %d bytes | Data: %s\n",
               peer_id, msg.chunk_id, msg.start_offset, msg.size, msg.data);
        total_chunks = msg.total_number;      
               
            }
        write_to_my_buff(msg);
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
        pthread_join(download_thread, NULL);
        if(total_chunks == total_downloaded){
            work_done(peer_id);
        }

    
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

    
    // Open the file for writing the downloaded chunks
    int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("Error opening file for writing");
        exit(1);
    }
    ChunkMessage *temp_buff = malloc(sizeof(ChunkMessage) * total_chunks);
    for (int i = 0; i < total_chunks; i++) {
        temp_buff[i] = buff[i];
    }
    

    qsort(temp_buff, total_chunks, sizeof(ChunkMessage), compare_chunks);
    // Write each downloaded chunk to the file
    for (int i = 0; i < total_chunks; i++) {
        {
            ssize_t bytes_written = write(fd, temp_buff[i].data, temp_buff[i].size);
            if (bytes_written == -1) {
                perror("Error writing to file");
                close(fd);
                exit(1);
            }
            printf("[PEER %d] is writing it's file...\n", peer_id);
        }
    }
    free(temp_buff);
    // Close the file after writing
    close(fd);
    
    // Map the shared memory to the process's address space
    status_ptr = (SharedStatus *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (status_ptr == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(1);
    }

    sem = sem_open("/my_sem", 0);
    sem_wait(sem);
    // Now, update the shared memory to indicate the peer has completed downloading
    status_ptr->peer_done[peer_id] = 1;  // 1 means completed
    printf("[PEER %d] Peer has completed downloading all chunks!\n", peer_id);
    close(shm_fd);
    sem_post(sem);
    sem_close(sem);
    
    
    char pipe_name[32];
    sprintf(pipe_name, "peer_pipe_%d", peer_id);

    int status;
    int fd;
    while(1){
        fd = open(pipe_name, O_RDONLY);
        if(read(fd, &status, sizeof(int)) > 0){
            if(status == 1){ // 1 means the tracker has permitted to exit the peer
                close(fd);
                printf("[PEER %d] Peer is exiting!\n", peer_id);
                exit(0);
            }
        }
        close(fd);
        usleep(500000);
    }
    //exit(0);
}

// void produce_chunk_in_my_buffer(ChunkMessage chunk) {
//     sem_wait(&shared_buffer.empty);
//     pthread_mutex_lock(&shared_buffer.mutex);

//     shared_buffer.buffer[shared_buffer.in] = chunk;
//     shared_buffer.in = (shared_buffer.in + 1) % MAX_BUFFER_SIZE;
//     shared_buffer.count++;

//     printf("Produced chunk %d (buffer count: %d)\n", chunk.chunk_id, shared_buffer.count);

//     pthread_mutex_unlock(&shared_buffer.mutex);
//     sem_post(&shared_buffer.full);
// }

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

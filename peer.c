#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/un.h>
#include <semaphore.h>

#define SOCKET_PATH "/tmp/mysocket"
#define MAX_BUFFER_SIZE 101

pthread_mutex_t mutex_lock_for_buffer = PTHREAD_MUTEX_INITIALIZER;
int total_downloaded = 0;
//int total_chunks = 0;
//sem_t *sem;
//SharedStatus *status_ptr; //pointer for shared memoryS
//sem_t reader, writer;
ChunkMessage buff[MAX_CHUNKS];

void* client_thread_func(void* arg) {
    char* peer_socket_path = (char*)arg;
    int client_fd;
    struct sockaddr_un addr;
    ChunkMessage msg;

    while (1) {
        sleep(2); // wait before sending

        // Connect to other peer
        client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0) {
            perror("socket");
            continue;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, peer_socket_path, sizeof(addr.sun_path) - 1);

        if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(client_fd);
            continue;
        }
        msg = buff[0];

        // Send message
        send(client_fd, &msg, sizeof(msg), 0);

        close(client_fd);
    }

    return NULL;
}

void* server_thread_func(void* arg) {
    int server_fd = *((int*)arg);
    //ChunkMessage buffer[MAX_CHUNKS];
    ChunkMessage msg;
    int client_fd;
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        while (1) {
            ssize_t bytes_received = recv(client_fd, &msg, sizeof(msg), 0);
            if (bytes_received > 0) {

                pthread_mutex_lock(&mutex_lock_for_buffer);
                write_to_my_buff(msg);// Received data ko combined buffer mein add
                pthread_mutex_unlock(&mutex_lock_for_buffer);

                printf("[SERVER] Received: Chunk %d\n", msg.chunk_id);
            } else {
                break; // Connection closed
            }
        }

        close(client_fd);
    }

    return NULL;
}

void write_to_my_buff(ChunkMessage msg){
    buff[total_downloaded++] = msg;
}

void run_peer(int peer_id, char peer_socket_paths[][108], int num){
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "peer_pipe_%d", peer_id);

    int fd = open(fifo_name, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open fifo (peer)");
        exit(1);
    }

    printf("[PEER %d] Started and waiting for chunks...\n", peer_id);

    ChunkMessage msg;

    if (read(fd, &msg, sizeof(msg)) != -1) {
        if(msg.chunk_id == -1){
            printf("[PEER %d] Received Empty Chunk (No initial chunk)..\n",
               peer_id);
        }
        else{
            printf("[PEER %d] Received Chunk %d | Offset: %d | Size: %d bytes | Data: %s\n",
                peer_id, msg.chunk_id, msg.start_offset, msg.size, msg.data);
            }
        write_to_my_buff(msg);
        total_chunks = msg.total_number;      
               
    }
        
    char my_socket_path[108];
    snprintf(my_socket_path, sizeof(my_socket_path), "peer_socket_%d", peer_id);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, my_socket_path, sizeof(addr.sun_path) - 1);
    unlink(my_socket_path); // Remove if already exists

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_thread_func, &server_fd);

    // Connect to all other peers
    pthread_t client_threads[num];
    for (int i = 0; i < num; i++) {
        if (i == peer_id) continue; // Skip self
        pthread_create(&client_threads[i], NULL, client_thread_func, (void*)peer_socket_paths[i]);
    }

    // Wait for server
    pthread_join(server_thread, NULL);
    //if(total_chunks == total_downloaded){
    work_done(peer_id);
    //}

    
}

int compare_chunks(const void *a, const void *b) {
    ChunkMessage *chunk_a = (ChunkMessage *)a;
    ChunkMessage *chunk_b = (ChunkMessage *)b;
    
    return chunk_a->chunk_id - chunk_b->chunk_id;
}

void work_done(int peer_id){
    char file_name[32];
    sprintf(file_name, "peer_%d_downloaded_file.txt", peer_id);
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
    ChunkMessage *temp_buff = malloc(sizeof(ChunkMessage) * total_downloaded);
    int c = 0;
    for (int i = 0; i < total_downloaded; i++, c++) {
        if (buff[i].chunk_id == -1)
        {
            continue;
        }
        
        temp_buff[c] = buff[i];
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

    while(1){
        fd = open(pipe_name, O_RDONLY);
        if(read(fd, &status, sizeof(int)) > 0){
            if(status == 1){ // 1 means the tracker has permitted to exit the peer
                close(fd);
                pthread_mutex_destroy(&mutex_lock_for_buffer);
                printf("[PEER %d] Peer is exiting!\n", peer_id);
                exit(0);
            }
        }
        close(fd);
        usleep(500000); //checking whether the tracker has called to exit this peer
    }
    //exit(0);
}



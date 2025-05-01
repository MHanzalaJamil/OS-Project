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



typedef struct {
    int peer_id;
    int num_peers;
    char peer_socket_paths[MAX_PEERS][108];
} ClientArgs;

pthread_mutex_t mutex_lock_for_buffer = PTHREAD_MUTEX_INITIALIZER;
int total_downloaded = 0;
int total_peers = 0;
// int total_chunks = 0;
// sem_t *sem;
// SharedStatus *status_ptr; //pointer for shared memoryS
// sem_t reader, writer;
ChunkMessage buff[MAX_CHUNKS];

void *client_thread_func(void *arg) {
    ClientArgs *args = (ClientArgs *)arg;
    int peer_id = args->peer_id;
    int num_peers = args->num_peers;

    for (int i = 0; i < num_peers; ++i) {
        if (i == peer_id) continue; // Skip self

        char *peer_socket_path = args->peer_socket_paths[i];
        int client_fd;
        struct sockaddr_un addr;
        ChunkMessage msg;

        sleep(1); // Optional delay

        client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0) {
            perror("socket");
            continue;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, peer_socket_path, sizeof(addr.sun_path) - 1);

        if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(client_fd);
            continue;
        }

        // Example: send first chunk
        pthread_mutex_lock(&mutex_lock_for_buffer);
        msg = buff[0];  // You can improve this by looping over all local chunks
        pthread_mutex_unlock(&mutex_lock_for_buffer);

        if (send(client_fd, &msg, sizeof(msg), 0) < 0) {
            perror("send");
        } else {
            printf("[CLIENT %d] Sent chunk %d to peer %d\n", peer_id, msg.chunk_id, i);
        }

        close(client_fd);
    }

    printf("[CLIENT %d] Done sending to all peers. Exiting.\n", peer_id);
    return NULL;
}


void *server_thread_func(void *arg)
{
    int server_fd = *((int *)arg);
    ChunkMessage msg;
    int client_fd;
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);


    
    int accepted_clients = 0;

    while (accepted_clients < total_peers - 1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        accepted_clients++; // increment when a new connection is accepted

        while (1)
        {
            ssize_t bytes_received = recv(client_fd, &msg, sizeof(msg), 0);
            if (bytes_received > 0)
            {
                pthread_mutex_lock(&mutex_lock_for_buffer);
                write_to_my_buff(msg); // Received data goes to combined buffer
                pthread_mutex_unlock(&mutex_lock_for_buffer);

                printf("[SERVER] Received: Chunk %d\n",  msg.chunk_id);
            }
            else
            {
                break; // Connection closed or error
            }
        }

        close(client_fd);
    }

    printf("[SERVER] Accepted all required peers. Exiting server loop.\n");
    return NULL;
}

void write_to_my_buff(ChunkMessage msg)
{
    
    
    buff[total_downloaded].chunk_id = msg.chunk_id;
    buff[total_downloaded].size = msg.size;
    buff[total_downloaded].start_offset = msg.start_offset;
    for (int i = 0; i < msg.size; i++)
    {
        buff[total_downloaded].data[i] = msg.data[i];
    }
    total_downloaded++;
}

void run_peer(int peer_id, char peer_socket_paths[][108], int num, int server_fd)
{
    
    //sleep(1);
    total_peers = num;
    ClientArgs box;
    box.num_peers = total_peers;
    box.peer_id = peer_id;
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < strlen(peer_socket_paths[i]); j++)
        {
            
            box.peer_socket_paths[i][j] = peer_socket_paths[i][j];
        }
        
    }
    
    
    char fifo_name[64];
    snprintf(fifo_name, sizeof(fifo_name), "peer_pipe_%d", peer_id);

    int fd = open(fifo_name, O_RDONLY);
    if (fd < 0)
    {
        perror("open fifo (peer)");
        exit(1);
    }

    printf("[PEER %d] Started and waiting for chunks...\n", peer_id);

    ChunkMessage msg;

    if (read(fd, &msg, sizeof(msg)) != -1)
    {
        if (msg.chunk_id == -1)
        {
            printf("[PEER %d] Received Empty Chunk (No initial chunk)..\n",
                   peer_id);
        }
        else
        {
            printf("[PEER %d] Received Chunk %d | Offset: %d | Size: %d bytes | Data: %s\n",
                   peer_id, msg.chunk_id, msg.start_offset, msg.size, msg.data);
        }
        write_to_my_buff(msg);
        total_chunks = msg.total_number;
    }

    
   

    printf("Starting Servers & Clients....\n");
    sleep(5 - peer_id);
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_thread_func, &server_fd);
    sleep(1);
    pthread_t client_threads;
    //for (int i = 0; i < num; i++)
    //{
        //if (i == peer_id) continue; // Skip self
    pthread_create(&client_threads, NULL, client_thread_func, (void *)&box);
    //}
    
    pthread_join(server_thread, NULL);
    //pthread_join(client_threads, NULL);
    //sleep(2);
    work_done(peer_id);

}

int compare_chunks(const void *a, const void *b)
{
    ChunkMessage *chunk_a = (ChunkMessage *)a;
    ChunkMessage *chunk_b = (ChunkMessage *)b;

    return chunk_a->chunk_id - chunk_b->chunk_id;
}

void work_done(int peer_id)
{
    char file_name[32];
    sprintf(file_name, "peer_%d_downloaded_file.txt", peer_id);
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0)
    {
        perror("Error opening shared memory");
        exit(1);
    }

    // Open the file for writing the downloaded chunks
    int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
    if (fd < 0)
    {
        perror("Error opening file for writing");
        exit(1);
    }
    ChunkMessage *temp_buff = malloc(sizeof(ChunkMessage) * total_downloaded);
    int c = 0;
    for (int i = 0; i < total_downloaded; i++, c++)
    {
        if (buff[i].chunk_id == -1)
        {
            continue;
        }

        temp_buff[c].chunk_id = buff[i].chunk_id;
        temp_buff[c].size = buff[i].size;
        temp_buff[c].start_offset = buff[i].start_offset;
        for (int j = 0; j < buff[i].size; j++)
        {
            temp_buff[c].data[j] = buff[i].data[j];
        }
        
    }

    qsort(temp_buff, total_chunks, sizeof(ChunkMessage), compare_chunks);
    // Write each downloaded chunk to the file
    for (int i = 0; i < total_chunks; i++)
    {
        //{
            ssize_t bytes_written = write(fd, temp_buff[i].data, temp_buff[i].size);
            if (bytes_written == -1)
            {
                perror("Error writing to file");
                close(fd);
                exit(1);
            }
            printf("[PEER %d] is writing it's file...\n", peer_id);
        //}
    }
    free(temp_buff);
    // Close the file after writing
    close(fd);

    // Map the shared memory to the process's address space
    status_ptr = (SharedStatus *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (status_ptr == MAP_FAILED)
    {
        perror("Error mapping shared memory");
        exit(1);
    }

    sem = sem_open("/my_sem", 0);
    sem_wait(sem);
    // Now, update the shared memory to indicate the peer has completed downloading
    status_ptr->peer_done[peer_id] = 1; // 1 means completed
    printf("[PEER %d] Peer has completed downloading all chunks!\n", peer_id);
    close(shm_fd);
    sem_post(sem);
    sem_close(sem);
    sleep(1);
    exit(0);
    //char pipe_name[32];
    //sprintf(pipe_name, "peer_pipe_%d", peer_id);

    // int status;

    // while (1)
    // {
    //     fd = open(pipe_name, O_RDONLY);
    //     if (read(fd, &status, sizeof(int)) > 0)
    //     {
    //         if (status == 1)
    //         { // 1 means the tracker has permitted to exit the peer
    //             close(fd);
    //             pthread_mutex_destroy(&mutex_lock_for_buffer);
    //             printf("[PEER %d] Peer is exiting!\n", peer_id);
    //             exit(0);
    //         }
    //     }
    //     close(fd);
    //     usleep(500000); // checking whether the tracker has called to exit this peer
    // }
    // exit(0);
}

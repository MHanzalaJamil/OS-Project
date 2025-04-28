#include "common.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>

total_chunks = 0;
registered_peers = 0;

// void split_file_into_chunks(const char* filename);

void assign_chunks_to_peer(int peer_id) //First Come First Serve basis
{
    char fifo_name[64];
    int fd;
    ChunkMessage msg;
    //char *pointer;

    snprintf(fifo_name, sizeof(fifo_name), "peer_pipe_%d", peer_id);
    fd = open(fifo_name, O_WRONLY);
    if (fd < 0)
    {
        perror("open fifo");
        return;
    }
    int check = 0;
    for (int i = 0; i < total_chunks; i++)
    {
        if (chunks[i].owner_peer_id == peer_id)
        {
            check = 1;
            msg.chunk_id = chunks[i].chunk_id;
            msg.start_offset = chunks[i].start_offset;
            msg.size = chunks[i].size;
            msg.data = chunks[i].data;
            msg.total_number = total_chunks;

            if (write(fd, &msg, sizeof(msg)) == -1)
            {
                perror("write to fifo");
            }
            else
            {
                printf("Tracker sent Chunk %d to Peer %d via %s\n",
                       msg.chunk_id, peer_id, fifo_name);
            }
        }
    }
    if(!check){
        msg.chunk_id = -1;
        msg.start_offset = -1;
        msg.size = -1;
        msg.data = '\0';
        msg.total_number = total_chunks;
        if (write(fd, &msg, sizeof(msg)) == -1)
        {
            perror("write to fifo");
        }
        else
        {
            printf("Tracker sent Blank Chunk to Peer %d via %s\n",
                    peer_id, fifo_name);
        }
    }
    close(fd);
}

void split_file_chunks_among_peers(const char *filename, int num)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        exit(1);
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        close(fd);
        exit(1);
    }
    int file_size = st.st_size;
    char buffer[file_size + 1];
    printf("File size: %d bytes\n", file_size);
    if(read(fd, buffer, sizeof(buffer)) == -1){
        perror("error read");
    }
    close(fd);
    // Calculate chunk size

    // if(file_size % MAX_PEERS != 0){
    //     int count = file_size % MAX_PEERS;
    //     file_size += (MAX_PEERS - count);
    // }
    int chunk_size = (file_size + num - 1) / num; // ceiling division
    total_chunks = (file_size + chunk_size - 1) / chunk_size;

    printf("Splitting into %d chunks (~%d bytes each)\n", total_chunks, chunk_size);
    int c = 0;
    for (int i = 0; i < total_chunks; ++i)
    {
        chunks[i].chunk_id = i;
        chunks[i].owner_peer_id = i % num;
        chunks[i].start_offset = i * chunk_size;
        chunks[i].size = (i == total_chunks - 1) ? (file_size - i * chunk_size) : chunk_size;

        for (int j = chunks[i].start_offset; j < (chunks[i].start_offset + chunks[i].size); j++)
        {
            chunks[i].data[c] = buffer[j];
            c++;
        }
        chunks[i].data[c] = '\0';
        
        //peer_chunks[chunks[i].owner_peer_id][chunks[i].chunk_id] = 1; // Updating to array

        printf("Chunk %d -> Peer %d, Offset: %d, Size: %d bytes\n",
               chunks[i].chunk_id,
               chunks[i].owner_peer_id,
               chunks[i].start_offset,
               chunks[i].size);
    }
}

int check_all_peers_done(SharedStatus *status_ptr)
{
    sem = sem_open("/my_sem", 0);
    sem_wait(sem);
    for (int i = 0; i < registered_peers; i++)
    {
        if (status_ptr->peer_done[i] == 0){
            sem_post(sem);
            return 0; // At least one peer not done
        }
    }
    sem_post(sem);
    return 1;
}

// void initialize_chunks(){
//     for (int i = 0; i < MAX_PEERS; i++)
//     {
//         for (int j = 0; j < MAX_CHUNKS; j++)
//         {
//             peer_chunks[i][j] = 0; // No chunk to any peer right now
//         }
        
//     }
    
// }

void run_tracker(int num)
{
    printf("[TRACKER] Tracker started\n");

    //initialize_chunks();
    create_pipes(num); // Named pipe for tracker
    split_file_chunks_among_peers("sample.txt", num);
    peer_registration(num);
    sem = sem_open("/my_sem", O_CREAT, 0666, 1);
    sem_wait(sem);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedStatus));
    status_ptr = (SharedStatus* )mmap(NULL, sizeof(SharedStatus), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    // Zero initialize
    for (int i = 0; i < registered_peers; i++)
    {
        status_ptr->peer_done[i] = 0;
    }
    sem_post(sem);
    sem_close(sem);
    for (int i = 0; i < registered_peers; i++)
    {
        assign_chunks_to_peer(i);
    }
    peer_deregistration(status_ptr, shm_fd); 
    // Open tracker pipe for reading peer registration
    // int tracker_fd = open(TRACKER_PIPE, O_RDONLY);
    // if (tracker_fd < 0) {
    //     perror("open tracker pipe");
    //     exit(1);
    // }
}
void peer_registration(int num)
{
    while (registered_peers < num)
    {
        printf("[TRACKER] Peer %d registered\n", registered_peers);
        registered_peers++;
    }
    usleep(500000);

    printf("[TRACKER] All peers registered.\n");
}

void peer_deregistration(SharedStatus *status_ptr, int shm_fd)
{
    while (1) {
        if(check_all_peers_done(status_ptr)){
            printf("[TRACKER] All peers completed. Exiting.\n");
            munmap(status_ptr, sizeof(SharedStatus));  // 1. Unmap the shared memory from your address space
            close(shm_fd);                             // 2. Close the file descriptor

            shm_unlink(SHM_NAME);  // Unlink the shared memory segment
            int fd = -1;    
            char pipe_name[32];                
            for (int i = 0; i < registered_peers; i++)
            {
                sprintf(pipe_name, "peer_pipe_%d", i);
                while (fd == -1)
                {
                    fd = open(pipe_name, O_WRONLY | O_NONBLOCK);
                }
                write(fd, 1, sizeof(int));
                close(fd);
                
            }
            exit(0);
        }
        usleep(500000);  // 0.5 second delay between checks
    }
    
    // printf("[TRACKER] Peer%d deregistered.\n", peer_id);
}



void create_pipes(int num)
{
    // Remove any old pipe that might exist
    // unlink(TRACKER_PIPE);

    // Create the named pipe for tracker to communicate with peers
    // if (mkfifo(TRACKER_PIPE, 0666) < 0) {
    //     perror("mkfifo tracker_pipe");
    //     exit(1);
    // }

    // Create pipes for each peer
    char pipe_name[32];
    for (int i = 0; i < num; ++i)
    {
        snprintf(pipe_name, sizeof(pipe_name), "peer_pipe_%d", i);

        // Remove old peer pipes if any
        unlink(pipe_name);

        // Create the named pipe for each peer
        if (mkfifo(pipe_name, 0666) < 0)
        {
            perror("mkfifo peer_pipe");
            exit(1);
        }
    }
    printf("[TRACKER] Pipes created\n");
}

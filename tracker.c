#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>


total_chunks = 0;
registered_peers = 0;

//void split_file_into_chunks(const char* filename);

void assign_chunks_to_peer(int peer_id) {
    char fifo_name[64];
    int fd;
    ChunkMessage msg;

    snprintf(fifo_name, sizeof(fifo_name), "peer_pipe_%d", peer_id);
    fd = open(fifo_name, O_WRONLY);
    if (fd < 0) {
        perror("open fifo");
        return;
    }

    for (int i = 0; i < total_chunks; i++) {
        if (chunks[i].owner_peer_id == peer_id) {
            msg.chunk_id = chunks[i].chunk_id;
            msg.start_offset = chunks[i].start_offset;
            msg.size = chunks[i].size;

            if (write(fd, &msg, sizeof(msg)) == -1) {
                perror("write to fifo");
            } else {
                printf("Tracker sent Chunk %d to Peer %d via %s\n",
                       msg.chunk_id, peer_id, fifo_name);
            }
        }
    }

    close(fd);
}


void split_file_chunks_among_peers(const char* filename, int num) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        exit(1);
    }
    int file_size = st.st_size;
    printf("File size: %d bytes\n", file_size);
    close(fd);

    // Calculate chunk size
    

    // if(file_size % MAX_PEERS != 0){
    //     int count = file_size % MAX_PEERS;
    //     file_size += (MAX_PEERS - count);
    // }
    int chunk_size = (file_size + num - 1) / num; // ceiling division
    total_chunks = (file_size + chunk_size - 1) / chunk_size; 

    printf("Splitting into %d chunks (~%d bytes each)\n", total_chunks, chunk_size);

    for (int i = 0; i < total_chunks; ++i) {
        chunks[i].chunk_id = i;
        chunks[i].owner_peer_id = i % num;
        chunks[i].start_offset = i * chunk_size;
        chunks[i].size = (i == total_chunks - 1) ? (file_size - i * chunk_size) : chunk_size;

        printf("Chunk %d -> Peer %d, Offset: %d, Size: %d bytes\n",
               chunks[i].chunk_id,
               chunks[i].owner_peer_id,
               chunks[i].start_offset,
               chunks[i].size);
        
           
    }

}

void run_tracker(int num) {
    printf("[TRACKER] Tracker started\n");
    create_pipes(num);  // Named pipe for tracker
    split_file_chunks_among_peers("sample.txt", num);
    peer_registration(num);
    for (int i = 0; i < registered_peers; i++)
    {
        assign_chunks_to_peer(i);
    }
    peer_deregistration(num); //kaam krna hai
    // Open tracker pipe for reading peer registration
    // int tracker_fd = open(TRACKER_PIPE, O_RDONLY);
    // if (tracker_fd < 0) {
    //     perror("open tracker pipe");
    //     exit(1);
    // }
}
void peer_registration(int num){
    while (registered_peers < num) {
            printf("[TRACKER] Peer %d registered\n", registered_peers);
            registered_peers++;
        }
        sleep(1);

    printf("[TRACKER] All peers registered.\n");
}

void peer_deregistration(int peer_id){
    //kaam krna hai
    printf("[TRACKER] Peer%d deregistered.\n", peer_id);
}

void create_pipes(int num) {
    // Remove any old pipe that might exist
    //unlink(TRACKER_PIPE);

    // Create the named pipe for tracker to communicate with peers
    // if (mkfifo(TRACKER_PIPE, 0666) < 0) {
    //     perror("mkfifo tracker_pipe");
    //     exit(1);
    // }

    // Create pipes for each peer
    char pipe_name[32];
    for (int i = 0; i < num; ++i) {
        snprintf(pipe_name, sizeof(pipe_name), "peer_pipe_%d", i);
        
        // Remove old peer pipes if any
        unlink(pipe_name);
        
        // Create the named pipe for each peer
        if (mkfifo(pipe_name, 0666) < 0) {
            perror("mkfifo peer_pipe");
            exit(1);
        }
    }
    printf("[TRACKER] Pipes created\n");
}

#ifndef COMMON_H
#define COMMON_H

// Constants
#define MAX_PEERS 4        // Max number of peers
#define MAX_CHUNKS 100     // Max number of chunks
#define SHM_NAME "/peer_status_shm"
#define SHM_SIZE 1024  // Can hold all peer messages

// int peer_chunks[MAX_PEERS][MAX_CHUNKS];

sem_t *sem; // semaphore for shared memory

typedef struct {
    int peer_done[MAX_PEERS];  // 0 or 1 (Shared Memory)
} SharedStatus;

SharedStatus *status_ptr; //pointer for shared memory

typedef struct {
    int chunk_id;
    int start_offset;
    char *data;
    int size;
    int total_number;
} ChunkMessage; //used for transfer of chunks

// Struct to hold information about a chunk
typedef struct {
    int chunk_id;         // ID of the chunk
    int owner_peer_id;    // Peer ID that owns this chunk
    char data[__INT_MAX__];
    int start_offset;  
    int size;  
} ChunkInfo; // chunk info holder


// Global variables to manage chunks and peers
int total_chunks;             // Total number of chunks in the file
int registered_peers;         // Counter for number of registered peers
ChunkInfo chunks[MAX_CHUNKS];  // Array to store chunks

// Function prototypes for tracker and peer operations
void split_file_chunks_among_peers(const char* filename, int num);
void assign_chunks_to_peer(int peer_id);
void create_pipes(int num); 
void run_tracker(int num); 
void peer_deregistration(SharedStatus *status_ptr, int shm_fd);

void run_peer(int peer_id);
void peer_registration(int num);
int compare_chunks(const void *a, const void *b);
void work_done(int peer_id);
void write_to_my_buff(ChunkMessage msg);

#endif

#ifndef COMMON_H
#define COMMON_H

// Constants
#define MAX_PEERS 4        // Max number of peers
#define MAX_CHUNKS 100     // Max number of chunks




typedef struct {
    int chunk_id;
    int start_offset;
    int size;
} ChunkMessage;

// Struct to hold information about a chunk
typedef struct {
    int chunk_id;         // ID of the chunk
    int owner_peer_id;    // Peer ID that owns this chunk
    //int is_downloaded;    // Whether this chunk has been downloaded by the peer
    int start_offset;  
    int size;  
} ChunkInfo;


// Global variables to manage chunks and peers
int total_chunks;             // Total number of chunks in the file
int registered_peers;         // Counter for number of registered peers
ChunkInfo chunks[MAX_CHUNKS];  // Array to store chunks

// Function prototypes for tracker and peer operations
void split_file_chunks_among_peers(const char* filename, int num);
void assign_chunks_to_peer(int peer_id);
void create_pipes(int num); 
void run_tracker(int num); 
void run_peer(int peer_id);
void peer_registration(int num);
void peer_deregistration(int peer_id);
void receive_chunk_assignment(int peer_id);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "common.h"

int total_chunks = 0;
SharedStatus *status_ptr = NULL;
sem_t* sem;

int main() {
    printf("[MAIN] Starting Torrent Tracker and Peers\n");
    printf("How many peers to start with? (MAX 4): ");
    int num;
    scanf("%d", &num);

    pid_t tracker_pid = fork();
    if (tracker_pid < 0) {
        perror("fork failed for tracker");
        exit(1);
    }
    
    if (tracker_pid == 0) {
        // Child: Tracker process
        run_tracker(num);
        //exit(0);
    }

    // Parent continues: Spawn peer processes
    sleep(1);  // give tracker a moment to start
    char peer_socket_path[num][108];
    //char buffer[100];
    for (int i = 0; i < num; i++)
    {

        sprintf(peer_socket_path[i], "peer_socket_%d", i);
        
        
    }
    
    for (int i = 0; i < num; ++i) {
        pid_t peer_pid = fork();
        if (peer_pid < 0) {
            perror("fork failed for peer");
            exit(1);
        }

        if (peer_pid == 0) {
            // Child: Peer process
            run_peer(i, peer_socket_path, num);
            // exit(0);
        }

        // Parent: continue loop to spawn next peer
        usleep(100000); // optional delay between peer launches
    }

    // Parent waits for all children
    for (int i = 0; i < num + 1; ++i) {
        wait(NULL);
    }

    printf("[MAIN] All processes finished.\n");
    return 0;
}

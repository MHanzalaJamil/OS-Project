#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "common.h"

int total_chunks = 0;
SharedStatus *status_ptr = NULL;
sem_t *sem;

int main()
{
    printf("[MAIN] Starting Torrent Tracker and Peers\n");
    printf("How many peers to start with?: ");
    int num;
    scanf("%d", &num);

    pid_t tracker_pid = fork();
    if (tracker_pid < 0)
    {
        perror("fork failed for tracker");
        exit(1);
    }

    if (tracker_pid == 0)
    {
        // Child: Tracker process
        run_tracker(num);
        // exit(0);
    }

    // Parent continues: Spawn peer processes
    sleep(1); // give tracker a moment to start
    char peer_socket_path[num][108];
    // char buffer[100];
    for (int i = 0; i < num; i++)
    {

        sprintf(peer_socket_path[i], "peer_socket_%d", i);
    }

    for (int i = 0; i < num; ++i)
    {
        pid_t peer_pid = fork();
        if (peer_pid < 0)
        {
            perror("fork failed for peer");
            exit(1);
        }

        if (peer_pid == 0)
        {
            // Child: Peer process
            char my_socket_path[108];
            snprintf(my_socket_path, sizeof(my_socket_path), "peer_socket_%d", i);
            int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (server_fd < 0)
            {
                perror("socket");
                exit(1);
            }

            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, my_socket_path, sizeof(addr.sun_path) - 1);
            unlink(my_socket_path); // Remove if already exists

            if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                perror("bind");
                exit(1);
            }

            if (listen(server_fd, 5) < 0)
            {
                perror("listen");
                exit(1);
            }
            run_peer(i, peer_socket_path, num, server_fd);

            // exit(0);
        }

        // Parent: continue loop to spawn next peer
        sleep(1); // optional delay between peer launches
    }

    // Parent waits for all children
    for (int i = 0; i < num + 1; ++i)
    {
        wait(NULL);
    }

    printf("[MAIN] All processes finished.\n");
    return 0;
}

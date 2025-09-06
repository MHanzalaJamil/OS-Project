# OS Project – Torrent Server/Client System  

## Overview  
This project is a simplified **peer-to-peer (P2P) Torrent System** implemented in **C** as part of an Operating Systems course.  
It demonstrates core OS concepts including:  

- Multithreading & Concurrency  
- Synchronization (mutexes and semaphores)  
- Inter-Process Communication (IPC) using named pipes (FIFOs)  
- Producer–Consumer & Reader–Writer problems  
- Deadlock avoidance  
- Tracker-based peer coordination  

The system allows files to be split into chunks, distributed among peers, and reassembled collaboratively through a **central tracker**.  

---

## Project Structure  
OS-Project/
│── tracker.c # Tracker process – manages peers & file chunk assignment
│── peer.c # Peer process – downloads/uploads chunks
│── main.c # Entry point / test harness
│── common.h # Shared structs, constants, and function prototypes
│── Makefile # Build automation
│── README.md # Project documentation

## Features  
- File chunking for distribution among peers  
- Peer registration and deregistration  
- Centralized tracker management  
- Thread synchronization for safe access  
- IPC using FIFOs (named pipes)  
- Deadlock prevention  

---

## Installation & Compilation  
Clone the repository:  
git clone https://github.com/MHanzalaJamil/OS-Project.git
cd OS-Project

# Build the project:
make
-- This generates executables for the tracker and peer programs.

# Usage
Run final exe file:
./project

# Example Workflow
A file is split into N chunks.
Tracker assigns chunks to peers in round-robin fashion.
Peers communicate via FIFOs to request and share chunks.
Once all chunks are received, peers merge them into the original file.

# Key Concepts Demonstrated
Process synchronization with mutexes & semaphores
Producer–Consumer model for file chunk distribution
Reader–Writer problem for shared file access
Named pipes (FIFOs) for IPC
Torrent-style file sharing in C

# Requirements
GCC compiler
Linux/Unix environment (POSIX threading and IPC support)

# Future Improvements
Peer-to-peer chunk exchange (bypassing tracker)
Parallel downloads
User-friendly CLI (upload, download, list)

# Author
Muhammad Hanzala Jamil
[muhaamadhanzalaj@gmail.com]

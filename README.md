# UT-OS-Course-Projects
Three comprehensive programming assignments from the Operating Systems course (Spring 2025) at University of Tehran — covering socket programming with TCP/UDP broadcast, multi-process pipelines with load balancing via pipes, and multi-threaded neural network inference on MNIST with pthread synchronization.

# Operating Systems Course Assignments

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-Ubuntu-brightgreen)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B-orange)]()
[![VM](https://img.shields.io/badge/VM-VMware%20Workstation-blue)]()

Three comprehensive programming assignments from the **Operating Systems** course (Spring 2025) at the **University of Tehran**, instructed by **Dr. Mehdi Kargahi**. These projects provide hands-on experience with fundamental operating systems concepts including socket programming, inter-process communication (IPC), multi-threading, and synchronization mechanisms — all implemented in a Linux environment.

---

## Table of Contents
1. [Project Overview](#project-overview)
2. [Assignments](#assignments)
   - [Assignment 1: Socket Programming Platform](#assignment-1-socket-programming-platform)
   - [Assignment 2: Process Pipeline & Load Balancing](#assignment-2-process-pipeline--load-balancing)
   - [Assignment 3: Multi-threaded Neural Network](#assignment-3-multi-threaded-neural-network)
3. [Project Structure](#project-structure)
4. [Requirements](#requirements)
5. [Build & Run](#build--run)
6. [Key Technical Concepts](#key-technical-concepts)
7. [License](#license)

---

## Project Overview

This repository contains my solutions to three operating systems programming assignments, each focusing on different core concepts of systems programming. All projects were developed and tested on **Ubuntu Linux** running inside **VMware Workstation**.

| Assignment | Focus Area | Key Topics |
|------------|------------|------------|
| **CA1** | Network Programming | Socket programming, TCP/UDP protocols, unicast & broadcast communication, `select()`/`poll()` multiplexing |
| **CA2** | Process Management | Process creation (`fork`), unnamed/named pipes (FIFO), pipeline processing, load balancing |
| **CA3** | Concurrency | Multi-threading (`pthread`), thread synchronization, semaphores, barriers, MNIST neural network inference |

---

## Assignments

### Assignment 1: Socket Programming Platform

**Goal:** Design and implement a competitive team-based programming contest platform using socket programming on IPv4.

The system operates as a **client-server platform** where:
- Teams consist of two members: a **Coder** and a **Navigator**
- The server manages team formation, distributes programming problems, and evaluates submissions
- Three predefined problems (addition, string reversal, palindrome checking) are solved in Python
- Each problem has a **1-minute time limit**

**Technical Implementation:**
- **TCP sockets** — Used for user registration, team communication, and code submissions (ensuring reliable, ordered data delivery)
- **UDP sockets** — Used for broadcasting problems and results to all teams (low overhead, efficient multi-receiver communication)
- **I/O Multiplexing** — Implemented using `select()`/`poll()` to manage concurrent client connections without busy-waiting
- **Evaluation System** — Integrated with an external code evaluation service

**Communication Strategy:**
- TCP → Reliable, connection-oriented communication for critical data
- UDP → Connectionless, high-speed communication for non-critical broadcasts

**Running the Components:**
```bash
# Start the server
./server <port>

# Launch a client (coder or navigator)
./client <username> <port> <role>   # role: coder or navigator
```
#### Output Screenshots:

* [Screenshot 1: Server startup and client connection handling]

* [Screenshot 2: Problem distribution and code submission flow]

### Assignment 2: Process Pipeline & Load Balancing
**Goal:** Build a **simplified ETL pipeline** to process Steam game data and calculate popularity rankings using inter-process communication via pipes.

This project implements a **multi-process pipeline** with three main stages:

#### Stage 1: Extraction & Transformation

* Read Steam game data from CSV files

* Filter irrelevant columns and clean data (remove currency symbols, parse numerical values)

* Convert categorical ratings to numerical scores (e.g., "Very Positive" → 6)

* Compute discount percentages and handle edge cases (0% discount → 1%)

#### Stage 2: Loading & Distribution

* Aggregate all filtered data at a load-balancing node

* Extract global min/max values for normalization

* Distribute data evenly to multiple compute nodes using **round-robin scheduling**

#### Stage 3: Network-Wide Computation

* Perform **Min-Max Scaling** on each column: X_scaled = ((X - X_min) / (X_max - X_min)) + 1

* Calculate popularity score using the formula:

```text
Score = (10 × Discount% × RecentRating × AllRating × RecentCount × TotalCount) / OriginalPrice
```
* Rank games by score and output to GameRanking.csv

#### Technical Implementation:

* **Unnamed Pipes (pipe()) —** Used for communication between related processes (parent-child)

* **Named Pipes (FIFOs) —** Used for communication between unrelated processes

* **Load Balancing —** Tasks distributed to compute nodes with the lowest CPU usage

* **Process Management —** Proper forking, waiting, and zombie process prevention using wait()/waitpid()

#### Process Architecture:

```text
Input CSV → [Extractor] → [Filter/Transform] → [Loader] → [Compute Node 1] → [Output]
                                          ├→ [Compute Node 2] → [Output]
                                          └→ [Compute Node N] → [Output]
```
#### Output Screenshots:

* [Screenshot 1: Pipeline execution showing data flow between processes]

* [Screenshot 2: Final GameRanking.csv output with sorted scores]

### Assignment 3: Multi-threaded Neural Network
**Goal:** Implement parallel inference for a neural network on the **MNIST handwritten digit dataset** using multi-threading and synchronization primitives.

#### Network Architecture:

* **Input Layer:** 784 neurons (28×28 pixels per image)

* **Hidden Layer:** 256 neurons with **ReLU** activation function

* **Output Layer:** 10 neurons with **Sigmoid** activation (digits 0–9)

#### Thread Allocation:

|Component|	Threads|	Responsibility|
|------------|------------|------------|
|Input Layer|	1 thread|	Read image pixels from t10k-images-idx3-ubyte (10,000 test images)
|Hidden Layer|	8 threads|	Compute activations for 32 neurons each (256 total)
|Output Layer|	10 threads|	Compute final scores for digits 0–9
|Prediction|	1 thread|	Determine max output and compare with actual labels

#### Synchronization Mechanisms:

* **Prioritization —** Output layer threads (highest priority) → hidden layer (medium) → input/preprocessing (standard)

* **Semaphores —** Coordinate data flow between layers

* **Barriers —** Ensure all threads in a layer complete before proceeding to the next layer

#### Technical Implementation:

* **POSIX Threads (pthread) —** For creating and managing all threads

* **Mutexes & Condition Variables —** Protect shared data structures from race conditions

* **Deadlock Prevention —** careful design of lock acquisition order and semaphore signaling

#### Dataset:

* **MNIST Database —** 70,000 handwritten digit images (60k training + 10k testing)

* Pre-trained weights and biases provided in net_params/ folder

#### Performance Comparison:
The parallel implementation demonstrates significant speedup over the provided serial version (serial.cpp), showcasing the benefits of multi-threading for neural network inference.

### Output Screenshots:

* [Screenshot 1: Parallel execution showing thread activity per layer]

* [Screenshot 2: Accuracy output and prediction results on test set]

## Project Structure
```text
OS-Course-Assignments/
├── CA1_Socket_Programming/
│   ├── server.cpp
│   ├── client.cpp
│   ├── Makefile
│   ├── OS-CA1-Description.pdf
│   └── screenshots/
├── CA2_Process_Pipeline/
│   ├── main.cpp
│   ├── extractor.cpp
│   ├── loader.cpp
│   ├── compute_node.cpp
│   ├── Makefile
│   ├── OS-CA2.pdf
│   └── screenshots/
├── CA3_Multi_Threaded_NN/
│   ├── parallel_nn.cpp
│   ├── serial.cpp
│   ├── Makefile
│   ├── OS-CA3.pdf
│   ├── data/               # MNIST dataset files
│   ├── net_params/         # Pre-trained weights and biases
│   └── screenshots/
└── README.md
```
## Requirements
* **Operating System:** Ubuntu Linux (tested on VMware Workstation)

* **Compiler:** g++ with C++11 or later

* **Build Tool:** make

* **Libraries:**

   * POSIX threads (pthread)

   * Standard C++ libraries

## Build & Run
#### Clone the Repository
```bash
git clone https://github.com/yourusername/Operating-Systems-Course-Assignments-UT-Spring-2025.git
cd Operating-Systems-Course-Assignments-UT-Spring-2025
```
## Assignment 1: Socket Platform
bash
cd CA1_Socket_Programming
make
./server 8080                     # Terminal 1
./client alice 8080 coder         # Terminal 2 (coder)
./client bob 8080 navigator       # Terminal 3 (navigator)
## Assignment 2: Process Pipeline
```bash
cd CA2_Process_Pipeline
make
./pipeline --input steam_games.csv --output GameRanking.csv --workers 4
```
## Assignment 3: Neural Network
```bash
cd CA3_Multi_Threaded_NN
make
# Serial version (baseline)
./serial
# Parallel multi-threaded version
./parallel_nn
```
#### Clean Build
```bash
make clean
make
```
## Key Technical Concepts

|Concept|	CA1|	CA2|	CA3|
|------------|------------|------------|------------|
|Socket Programming|	✓ TCP/UDP|	—|	—|
|I/O Multiplexing|	✓ select/poll|	—|	—|
|Process Creation|	—	|✓ fork|	—|
|Unnamed Pipes|	—|	✓|	—|
|Named Pipes (FIFO)|	—	|✓|	—|
|Multi-threading|	—|	—	|✓ pthread|
|Semaphores|	—	|—|	✓|
|Barriers|	—|	—|	✓|
|Load Balancing|	—|	✓	|—|
|Data Pipeline (ETL)|	—	|✓	|—|
|Neural Networks|	—|	—	|✓ MNIST|
## License
This project is licensed under the MIT License — see the LICENSE file for details.

## Acknowledgements
* **Dr. Mehdi Kargahi —** Course instructor, University of Tehran

* **Teaching Assistants:** Ali Eshghi Movahed, Sayhan Alaaldini, Behrad Elmi, Mohammad Amanlou, Ali Ghanbari, Mahdi Hajisayed Hossein

* **VMware Workstation —** Virtualization platform

* **MNIST Database —** Yann LeCun, Corinna Cortes, Christopher J.C. Burges

<div align="center">
📍 University of Tehran — Spring 2025

</div> 

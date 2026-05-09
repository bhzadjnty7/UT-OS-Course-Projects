#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>  // برای استفاده از strdup

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <num_compute_processes> <csv_file1> [csv_file2] ..." << endl;
        return 1;
    }
    
    int num_compute_processes = atoi(argv[1]);
    int num_files = argc - 2;
    
    if (num_compute_processes <= 0) {
        cerr << "Number of compute processes must be greater than 0" << endl;
        return 1;
    }
    
    if (num_files <= 0) {
        cerr << "At least one input file must be provided" << endl;
        return 1;
    }
    
    // Create pipes for extract_transform -> load
    int** extract_to_load_pipes = new int*[num_files];
    for (int i = 0; i < num_files; i++) {
        extract_to_load_pipes[i] = new int[2];
        if (pipe(extract_to_load_pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    
    // Create pipes for load -> compute
    int** load_to_compute_pipes = new int*[num_compute_processes];
    for (int i = 0; i < num_compute_processes; i++) {
        load_to_compute_pipes[i] = new int[2];
        if (pipe(load_to_compute_pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    
    // Create pipe for compute -> output
    int compute_to_output_pipe[2];
    if (pipe(compute_to_output_pipe) == -1) {
        perror("pipe");
        return 1;
    }
    
    cout << "Starting extraction and transformation processes..." << endl;
    
    // Fork extract_transform processes
    pid_t* extract_pids = new pid_t[num_files];
    for (int i = 0; i < num_files; i++) {
        extract_pids[i] = fork();
        
        if (extract_pids[i] < 0) {
            perror("fork");
            return 1;
        } else if (extract_pids[i] == 0) {
            // Child process (extract_transform)
            
            // Close read end of current pipe
            close(extract_to_load_pipes[i][0]);
            
            // Close all other pipes
            for (int j = 0; j < num_files; j++) {
                if (j != i) {
                    close(extract_to_load_pipes[j][0]);
                    close(extract_to_load_pipes[j][1]);
                }
            }
            
            for (int j = 0; j < num_compute_processes; j++) {
                close(load_to_compute_pipes[j][0]);
                close(load_to_compute_pipes[j][1]);
            }
            
            close(compute_to_output_pipe[0]);
            close(compute_to_output_pipe[1]);
            
            // Convert file descriptor to string
            string fd_str = to_string(extract_to_load_pipes[i][1]);
            
            cout << "Executing extract_transform for file: " << argv[i + 2] << endl;
            
            // Execute extract_transform program
            execlp("./extract_transform", "extract_transform", argv[i + 2], fd_str.c_str(), nullptr);
            
            // If exec fails
            perror("execlp extract_transform");
            exit(1);
        }
        
        // Parent process closes write end of pipe
        close(extract_to_load_pipes[i][1]);
    }
    
    cout << "Starting load process..." << endl;
    
    // Fork load process
    pid_t load_pid = fork();
    
    if (load_pid < 0) {
        perror("fork");
        return 1;
    } else if (load_pid == 0) {
        // Child process (load)
        
        // Prepare file descriptors for extract_transform pipes
        vector<string> extract_fds;
        for (int i = 0; i < num_files; i++) {
            extract_fds.push_back(to_string(extract_to_load_pipes[i][0]));
        }
        
        // Prepare file descriptors for compute pipes
        vector<string> compute_fds;
        for (int i = 0; i < num_compute_processes; i++) {
            close(load_to_compute_pipes[i][0]);
            compute_fds.push_back(to_string(load_to_compute_pipes[i][1]));
        }
        
        close(compute_to_output_pipe[0]);
        close(compute_to_output_pipe[1]);
        
        // Convert vector to array of char*
        char** args = new char*[3 + extract_fds.size() + compute_fds.size()];
        args[0] = strdup("load");
        args[1] = strdup(to_string(num_files).c_str());
        args[2] = strdup(to_string(num_compute_processes).c_str());
        
        int arg_index = 3;
        for (const auto& fd : extract_fds) {
            args[arg_index++] = strdup(fd.c_str());
        }
        
        for (const auto& fd : compute_fds) {
            args[arg_index++] = strdup(fd.c_str());
        }
        
        args[arg_index] = nullptr;
        
        cout << "Executing load process" << endl;
        
        // Execute load program
        execvp("./load", args);
        
        // If exec fails
        perror("execvp load");
        
        // Free allocated memory
        for (int i = 0; i < arg_index; i++) {
            free(args[i]);
        }
        delete[] args;
        
        exit(1);
    }
    
    // Parent closes extract_transform read pipes
    for (int i = 0; i < num_files; i++) {
        close(extract_to_load_pipes[i][0]);
    }
    
    // Parent closes load_to_compute write pipes
    for (int i = 0; i < num_compute_processes; i++) {
        close(load_to_compute_pipes[i][1]);
    }
    
    cout << "Starting compute processes..." << endl;
    
    // Fork compute processes
    pid_t* compute_pids = new pid_t[num_compute_processes];
    for (int i = 0; i < num_compute_processes; i++) {
        compute_pids[i] = fork();
        
        if (compute_pids[i] < 0) {
            perror("fork");
            return 1;
        } else if (compute_pids[i] == 0) {
            // Child process (compute)
            
            // Close all other compute read pipes
            for (int j = 0; j < num_compute_processes; j++) {
                if (j != i) {
                    close(load_to_compute_pipes[j][0]);
                }
            }
            
            close(compute_to_output_pipe[0]);
            
            cout << "Executing compute process " << i << endl;
            
            // Execute compute program
            execlp("./compute", "compute", 
                   to_string(load_to_compute_pipes[i][0]).c_str(), 
                   to_string(compute_to_output_pipe[1]).c_str(), 
                   to_string(i).c_str(), 
                   nullptr);
            
            // If exec fails
            perror("execlp compute");
            exit(1);
        }
        
        // Parent closes compute read pipe
        close(load_to_compute_pipes[i][0]);
    }
    
    cout << "Starting output process..." << endl;
    
    // Fork output process
    pid_t output_pid = fork();
    
    if (output_pid < 0) {
        perror("fork");
        return 1;
    } else if (output_pid == 0) {
        // Child process (output)
        
        close(compute_to_output_pipe[1]);
        
        cout << "Executing output process" << endl;
        
        // Execute output program
        execlp("./output", "output", 
               to_string(compute_to_output_pipe[0]).c_str(), 
               to_string(num_compute_processes).c_str(), 
               nullptr);
        
        // If exec fails
        perror("execlp output");
        exit(1);
    }
    
    // Parent closes compute_to_output pipes
    close(compute_to_output_pipe[0]);
    close(compute_to_output_pipe[1]);
    
    cout << "Waiting for all processes to complete..." << endl;
    
    // Wait for all child processes to finish
    for (int i = 0; i < num_files; i++) {
        waitpid(extract_pids[i], nullptr, 0);
        cout << "Extract process " << i << " completed" << endl;
    }
    
    waitpid(load_pid, nullptr, 0);
    cout << "Load process completed" << endl;
    
    for (int i = 0; i < num_compute_processes; i++) {
        waitpid(compute_pids[i], nullptr, 0);
        cout << "Compute process " << i << " completed" << endl;
    }
    
    waitpid(output_pid, nullptr, 0);
    cout << "Output process completed" << endl;
    
    // Free allocated memory
    for (int i = 0; i < num_files; i++) {
        delete[] extract_to_load_pipes[i];
    }
    delete[] extract_to_load_pipes;
    
    for (int i = 0; i < num_compute_processes; i++) {
        delete[] load_to_compute_pipes[i];
    }
    delete[] load_to_compute_pipes;
    
    delete[] extract_pids;
    delete[] compute_pids;
    
    cout << "All processes completed successfully." << endl;
    
    return 0;
}
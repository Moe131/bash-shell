#include "linkedlist.h"
#include "icssh.h"
#include <string.h>

// Your helper functions need to be here.
int is_builtin_command(char* line){
    	if (strcmp(line, "exit") == 0)
            return 1;
		else if (strcmp(line, "cd") == 0)
            return 1;
    	else if (strcmp(line, "estatus") == 0)
            return 1;
    	else if (strcmp(line, "bglist") == 0)
            return 1;
        else if (strcmp(line, "fg") == 0)
            return 1;
        else 
            return 0;
}



int handle_exit_command(job_info* job, list_t* bg_job_list){
            // Terminate all background jobs before exiting
            node_t* current = bg_job_list->head;
            while (current != NULL) {
                bgentry_t* bg_entry = (bgentry_t*)current->data;
                printf(BG_TERM, bg_entry->pid, bg_entry->job->line);  
                kill(bg_entry->pid, SIGTERM); 
                free_job(bg_entry->job);
                free(bg_entry);
                current = current->next;
            }

            // Clean up and exit
            DeleteList(bg_job_list);
            free(bg_job_list);
            free_job(job);
            validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
            return 0;

}


void handle_cd_command(job_info* job){
			// change directory to HOME
			if (job->procs->argc == 1) {
				char* homeDir = getenv("HOME");
				if (chdir(homeDir) != 0)
					fprintf(stderr, DIR_ERR);
				else { // successful
					char* cwd = getcwd(NULL, 0);
					if (cwd != NULL) {
						printf("%s\n", cwd);
						free(cwd);
					} else {
						fprintf(stderr, DIR_ERR);
					}
				}
			}
			// change directory to the path provided
			else {
				char* targetPath = job->procs->argv[1];
				if (chdir(targetPath) != 0)
					fprintf(stderr, DIR_ERR);
				else { // successful
					char* cwd = getcwd(NULL, 0);
					if (cwd != NULL) {
						printf("%s\n", cwd);
						free(cwd);
					} else {
						fprintf(stderr, DIR_ERR);
					}
				}
			}
			free_job(job);
}


void handle_estatus_command(job_info* job, int last_child_status){
			// prints the exit status of the most recent reaped program (aka child process)
            printf("%d\n",last_child_status);
			free_job(job);
}

void handle_bglist_command(job_info* job, list_t* bg_job_list){

            PrintLinkedList(bg_job_list, stderr);
			free_job(job);
}



int compare_bgentry(const void* a, const void* b) {
    const bgentry_t* bg1 = (const bgentry_t*)a;
    const bgentry_t* bg2 = (const bgentry_t*)b;
    return (bg2->seconds - bg1->seconds);  // Most recent first
}

// Helper function to find a background job by PID 
bgentry_t* find_bg_job_by_pid(list_t* bg_job_list, pid_t pid) {
    node_t* current = bg_job_list->head;
    while (current != NULL) {
        bgentry_t* entry = (bgentry_t*)current->data;
        if (entry->pid == pid) {
            return entry;
        }
        current = current->next;
    }
    return NULL; 
}


void remove_process_from_list(list_t* bg_job_list, pid_t pid) {
    node_t* current = bg_job_list->head;
    int index = 0;
    while (current != NULL) {
        bgentry_t* entry = (bgentry_t*)current->data;
        if (entry->pid == pid) {
            RemoveByIndex(bg_job_list, index);
            free_job(entry->job);
            free(entry);
            break;
        }
        current = current->next;
        index++;
    }
}


void reap_terminated_children(list_t* bg_job_list, int* child_terminated, int* last_child_status) {
    int status;
    pid_t pid;
    // Reap each terminated child one at a time
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        bgentry_t* entry = find_bg_job_by_pid(bg_job_list,pid);
        printf(BG_TERM, pid, entry->job->line);
        remove_process_from_list(bg_job_list, pid);

        if (WIFEXITED(status)) 
            *last_child_status = WEXITSTATUS(status);  // Update status if exited normally
        
    }
    // Reset the flag after all terminated children have been reaped
    *child_terminated = 0;
}



void handle_fg_command(job_info* job, list_t* bg_job_list) {
            pid_t pid;
         // bring the most recent bg process
            if (bg_job_list->length == 0){
                fprintf(stderr, PID_ERR);
            }
			else if (job->procs->argc == 1) {
                int status;
                bgentry_t* bg = bg_job_list->head->data;
                if (bg == NULL){
                    fprintf(stderr, PID_ERR);
                }
                else {
                    pid_t bg_pid = bg->pid ;
                    printf("%s\n", bg->job->line);
    				if ((pid = waitpid(bg_pid, &status, 0)) < 0) {
                        fprintf(stderr, PID_ERR);
                    }
                    remove_process_from_list(bg_job_list, bg_pid);
                }
			}
                
            // bring the bg process with given PID
			else {
                int status;
                pid_t bg_pid = atoi(job->procs->argv[1]);
                bgentry_t* bg = find_bg_job_by_pid(bg_job_list, bg_pid);
                if (bg == NULL){
                    fprintf(stderr, PID_ERR);
                }
                else {
                    printf("%s\n", bg->job->line);
    				if ((pid = waitpid(bg->pid, &status, 0)) < 0) {
                        fprintf(stderr, PID_ERR);
                    }
                    remove_process_from_list(bg_job_list, bg->pid);
                }
			}
			free_job(job);
}


void handle_bg_process(job_info* job, list_t* bg_job_list, pid_t pid) {
        // Create a new bgentry_t for the job
        bgentry_t* new_bg = malloc(sizeof(bgentry_t));
        new_bg->job = job;
        new_bg->pid = pid;
        new_bg->seconds = time(NULL);

        // Insert into the background job list
        InsertInOrder(bg_job_list, new_bg);
}

void handle_fg_process(job_info* job, list_t* bg_job_list, int* last_child_status, 	pid_t pid) {
        int status;
    	pid_t wait_result = waitpid(pid, &status, 0);
    	if (wait_result < 0) {
    		printf(WAIT_ERR);
    		exit(EXIT_FAILURE);
    		}
            // Update last_child_status based on child's exit status
        *last_child_status = WEXITSTATUS(status);
}

void execute_child_process(job_info* job){

    // Check for redirection conflicts
    if ((job->in_file && job->out_file && strcmp(job->in_file, job->out_file) == 0) ||
        (job->in_file && job->procs->err_file && strcmp(job->in_file, job->procs->err_file) == 0) ||
        (job->out_file && job->procs->err_file && strcmp(job->out_file, job->procs->err_file) == 0)) {
        fprintf(stderr, RD_ERR);  // Same file shared for multiple types of redirection
        free_job(job);  
        validate_input(NULL);
        exit(EXIT_FAILURE);
    }
    
    int fd;
        // Handle input redirection
    if (job->in_file) {
        fd = open(job->in_file, O_RDONLY, 0);
        if (fd < 0) {
            fprintf(stderr, RD_ERR);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("Error duplicating file descriptor for input");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);
    }

    // Handle output redirection
    if (job->out_file) {
        fd = open(job->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Error duplicating file descriptor for output");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);
    }

    // Handle error redirection
    if (job->procs->err_file) {
        fd = open(job->procs->err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Error opening error file");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("Error duplicating file descriptor for error");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);
    }

    
    int exec_result;
	proc_info* proc = job->procs;
	exec_result = execvp(proc->cmd, proc->argv);
	if (exec_result < 0) {  //Error checking
		printf(EXEC_ERR, proc->cmd);
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent)
		free_job(job);  
    	validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

		exit(EXIT_FAILURE);
	}   
}


// Function to execute jobs with a single pipe (two commands)
void handle_single_pipe(job_info* job, int* last_child_status, list_t* bg_job_list) {
    int p[2];
    pid_t pid1, pid2;

    if (pipe(p) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    proc_info *proc1 = job->procs;
    proc_info *proc2 = proc1->next_proc;

    // First process in the pipeline
    if ((pid1 = fork()) == 0) {
        close(p[0]); // Close unused read end
        dup2(p[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
        close(p[1]);

        // Execute first command in the job
        execvp(proc1->cmd, proc1->argv);
        perror("execvp failed for first command");
        exit(EXIT_FAILURE);
    } else if (pid1 < 0) {
        perror("fork failed for first command");
        exit(EXIT_FAILURE);
    }

    // Second process in the pipeline
    if ((pid2 = fork()) == 0) {
        close(p[1]); // Close unused write end
        dup2(p[0], STDIN_FILENO); // Redirect stdin to read end of pipe
        close(p[0]);

        // Execute second command in the job
        execvp(proc2->cmd, proc2->argv);
        perror("execvp failed for second command");
        exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
        perror("fork failed for second command");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(p[0]);
    close(p[1]);

    int status1, status2;
    if (job->bg) {
        // If it's a background job, add it to the list and return
        waitpid(pid1, &status1, 0);
        handle_bg_process(job, bg_job_list, pid2);
        return; 
    }

    // Wait for both child processes to complete
    waitpid(pid1, &status1, 0);
    waitpid(pid2, last_child_status, 0); // Capture the last command status

    free_job(job);
}


// Function to execute jobs with two pipes (three commands)
void handle_two_pipes(job_info* job, int* last_child_status, list_t* bg_job_list) {
    int p1[2], p2[2];
    pid_t pid1, pid2, pid3;

    // Create two pipes for the three commands
    if (pipe(p1) == -1 || pipe(p2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    proc_info *proc1 = job->procs;
    proc_info *proc2 = proc1->next_proc;
    proc_info *proc3 = proc2->next_proc;

    // First process in the pipeline
    if ((pid1 = fork()) == 0) {
        close(p1[0]); // Close unused read end of first pipe
        dup2(p1[1], STDOUT_FILENO); // Redirect stdout to write end of first pipe
        close(p1[1]);

        // Execute first command in the job
        execvp(proc1->cmd, proc1->argv);
        perror("execvp failed for first command");
        exit(EXIT_FAILURE);
    } else if (pid1 < 0) {
        perror("fork failed for first command");
        exit(EXIT_FAILURE);
    }

    // Second process in the pipeline
    if ((pid2 = fork()) == 0) {
        close(p1[1]); // Close unused write end of first pipe
        dup2(p1[0], STDIN_FILENO); // Redirect stdin to read end of first pipe
        close(p1[0]);

        close(p2[0]); // Close unused read end of second pipe
        dup2(p2[1], STDOUT_FILENO); // Redirect stdout to write end of second pipe
        close(p2[1]);

        // Execute second command in the job
        execvp(proc2->cmd, proc2->argv);
        perror("execvp failed for second command");
        exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
        perror("fork failed for second command");
        exit(EXIT_FAILURE);
    }

    // Third process in the pipeline
    if ((pid3 = fork()) == 0) {
        close(p1[0]); // Close unused read end of first pipe
        close(p1[1]); // Close unused write end of first pipe
        close(p2[1]); // Close unused write end of second pipe
        dup2(p2[0], STDIN_FILENO); // Redirect stdin to read end of second pipe
        close(p2[0]);

        // Execute third command in the job
        execvp(proc3->cmd, proc3->argv);
        perror("execvp failed for third command");
        exit(EXIT_FAILURE);
    } else if (pid3 < 0) {
        perror("fork failed for third command");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);

    int status1, status2, status3;
    if (job->bg) {
        // If it's a background job, add it to the list and return
        waitpid(pid1, &status1, 0);
        waitpid(pid2, &status2, 0);
        handle_bg_process(job, bg_job_list, pid3);
        return; 
    }

    // Wait for all child processes to complete
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    waitpid(pid3, last_child_status, 0); // Capture the last command status

    free_job(job);
}


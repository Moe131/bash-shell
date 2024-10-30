#include "icssh.h"
#include <readline/readline.h>

int last_child_status = 0;
int child_terminated = 0;

int compare_bgentry(const void* a, const void* b) {
    const bgentry_t* bg1 = (const bgentry_t*)a;
    const bgentry_t* bg2 = (const bgentry_t*)b;
    return (bg2->seconds - bg1->seconds);  // Most recent first
}

void handle_bg_completion(list_t* bg_job_list, pid_t pid) {
    node_t* current = bg_job_list->head;
    int index = 0;
    while (current != NULL) {
        bgentry_t* entry = (bgentry_t*)current->data;
        if (entry->pid == pid) {
            RemoveByIndex(bg_job_list, index);
            printf(BG_TERM, pid, entry->job->line);
            free(entry->job->line);
            free(entry->job);
            free(entry);
            break;
        }
        current = current->next;
        index++;
    }
}

void sigchld_handler(int sig) {
    // Signal handler for SIGCHLD, sets the flag to indicate a child has terminated
    child_terminated = 1;
}

void reap_terminated_children(list_t* bg_job_list) {
    int status;
    pid_t pid;
    // Reap each terminated child one at a time
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        handle_bg_completion(bg_job_list, pid);
        if (WIFEXITED(status)) 
            last_child_status = WEXITSTATUS(status);  // Update status if exited normally
        
    }
    // Reset the flag after all terminated children have been reaped
    child_terminated = 0;
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



int main(int argc, char* argv[]) {
    int max_bgprocs = -1;
	int exec_result;
	int exit_status;
    list_t* bg_job_list = NULL;
	pid_t pid;
	pid_t wait_result;
	char* line;
#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

    bg_job_list = CreateList(compare_bgentry, print_bgentry, free);


    // check command line arg
    if(argc > 1) {
        int check = atoi(argv[1]);
        if(check != 0)
            max_bgprocs = check;
        else {
            printf("Invalid command line argument value\n");
            exit(EXIT_FAILURE);
        }
    }

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

    // Setup the SIGCHLD handler
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) {
        perror("Failed to set SIGCHLD handler");
        exit(EXIT_FAILURE);
    }

    	// print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
            // Check flag to reap all the terminated bg processes 
            if (child_terminated)
                reap_terminated_children(bg_job_list);
        
        	// MAGIC HAPPENS! Command string is parsed into a job struct
        	// Will print out error message if command string is invalid
		    job_info* job = validate_input(line);
        	if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}

        	//Prints out the job linked list struture for debugging
        	#ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
            		debug_print_job(job);
        	#endif

		// example built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			// Terminating the shell
			free(line);
			free_job(job);
            validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
            return 0;
		}

		//  built-in: cd
		if (strcmp(job->procs->cmd, "cd") == 0) {
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
			free(line);
			free_job(job);
			continue;
		}


        //  built-in: estatus 
		if (strcmp(job->procs->cmd, "estatus") == 0) {
			// prints the exit status of the most recent reaped program (aka child process)
            printf("%d\n",last_child_status);
			free(line);
			free_job(job);
			continue;
		}

        //  built-in: bglist 
		if (strcmp(job->procs->cmd, "bglist") == 0) {
            PrintLinkedList(bg_job_list, stderr);
            free(line);
			free_job(job);
			continue;
		}

        
        //  built-in: fg 
		if (strcmp(job->procs->cmd, "fg") == 0) {
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
                    handle_bg_completion(bg_job_list, bg_pid);
                }
			}
                
            // bring the most  bg processwith given PID
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
                    handle_bg_completion(bg_job_list, bg->pid);
                }
			}
			free(line);
			free_job(job);
            continue;
		}


        // Background process but maximum is reached
        if (job->bg && bg_job_list->length >= max_bgprocs) {
            fprintf(stderr, BG_ERR);
            free(line);
            free_job(job);
            continue;
        } 
        
		// example of good error handling!
        // create the child proccess
		if ((pid = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {  //If zero, then it's the child process
            //get the first command in the job list to execute
		    proc_info* proc = job->procs;
			exec_result = execvp(proc->cmd, proc->argv);
			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent)
				free_job(job);  
				free(line);
    				validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

				exit(EXIT_FAILURE);
			}
		} else {
        	// As the parent, wait for the foreground job to finish

            // background process
            if (job->bg) {
                // Create a new bgentry_t for the job
                bgentry_t* new_bg = malloc(sizeof(bgentry_t));
                new_bg->job = job;
                new_bg->pid = pid;
                new_bg->seconds = time(NULL);

                // Insert into the background job list
                InsertInOrder(bg_job_list, new_bg);
                printf("Started background job with PID: %d\n", pid);
                continue;
            }
            // foreground process
            else {
    			wait_result = waitpid(pid, &exit_status, 0);
    			if (wait_result < 0) {
    				printf(WAIT_ERR);
    				exit(EXIT_FAILURE);
    			}
                // Update last_child_status based on child's exit status
                last_child_status = WEXITSTATUS(exit_status);
            } 
		}

		free_job(job);  // if a foreground job, we no longer need the data
		free(line);
	}

    	// calling validate_input with NULL will free the memory it has allocated
    	validate_input(NULL);
        DeleteList(bg_job_list);


#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}

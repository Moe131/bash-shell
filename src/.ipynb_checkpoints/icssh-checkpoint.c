#include "icssh.h"
#include <readline/readline.h>

int main(int argc, char* argv[]) {
    int max_bgprocs = -1;
	int exec_result;
	int exit_status;
    int last_child_status = 0;
	pid_t pid;
	pid_t wait_result;
	char* line;
#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

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

    	// print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
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
			wait_result = waitpid(pid, &exit_status, 0);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
            // Update last_child_status based on child's exit status
            last_child_status = WEXITSTATUS(exit_status);
		}

		free_job(job);  // if a foreground job, we no longer need the data
		free(line);
	}

    	// calling validate_input with NULL will free the memory it has allocated
    	validate_input(NULL);

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}

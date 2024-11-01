#include "icssh.h"
#include "linkedlist.h"
#include "helpers.h"

#include <readline/readline.h>

int last_child_status = 0;
int child_terminated = 0;


void sigchld_handler(int sig) {
    // Signal handler for SIGCHLD, sets the flag to indicate a child has terminated
    child_terminated = 1;
}

void sigusr2_handler(int sig) {
    time_t now = time(NULL);    
    char* date_str = ctime(&now);   
    if (date_str != NULL) {
        write(STDERR_FILENO, date_str, strlen(date_str));
    }
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
     // Setup the SIGUSR2 handler
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR) {
        perror("Failed to set SIGUSR2 handler");
        exit(EXIT_FAILURE);
    }

    	// print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
        
            // Check flag to reap all the terminated bg processes 
            if (child_terminated)
                reap_terminated_children(bg_job_list, &child_terminated, &last_child_status);
        
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


        // Background process but maximum is reached
        if (job->bg && bg_job_list->length >= max_bgprocs && max_bgprocs != -1) {
            fprintf(stderr, BG_ERR);
            free(line);
            free_job(job);
            continue;
        } 

        // Check if it's a piped command with exactly 2 processes
        if (job->nproc > 1) {
           if (job->nproc == 2) 
                handle_single_pipe(job, &last_child_status, bg_job_list);
            else if (job->nproc == 3) 
                handle_two_pipes(job, &last_child_status, bg_job_list);
            free(line);
        }
            
        
        // built in command
        else if (is_builtin_command(job->procs->cmd)){
            if (strcmp(job->procs->cmd, "exit") == 0) {
                free(line);
                return handle_exit_command(job ,bg_job_list);
            }
            else if (strcmp(job->procs->cmd, "cd") == 0)
                handle_cd_command(job);
            else if (strcmp(job->procs->cmd, "estatus") == 0)
                handle_estatus_command(job, last_child_status);
            else if (strcmp(job->procs->cmd, "bglist") == 0)
                handle_bglist_command(job, bg_job_list);
            else if (strcmp(job->procs->cmd, "fg") == 0)
                handle_fg_command(job, bg_job_list);
            free(line);
        }
            
        // Not built in command
        else {
            // create the child proccess
    		if ((pid = fork()) < 0) {
    			perror("fork error");
    			exit(EXIT_FAILURE);
    		}

            // If zero, then it's the child process
    		if (pid == 0) {  
                execute_child_process(job);
                free(line);
    		}
            // Its a parent process
            else {  
                if (job->bg)  // background process  
                    handle_bg_process(job, bg_job_list, pid);

                else  {     // foreground process     
                    handle_fg_process(job, bg_job_list, &last_child_status, pid);
                    free_job(job);
                }
                free(line);
        	}
        }
        
	}

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}

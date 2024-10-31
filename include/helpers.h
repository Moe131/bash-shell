// A header file for helpers.c
// Declare any additional functions in this file
#include "linkedlist.h"
#include "icssh.h"
#include <string.h>



int is_builtin_command(char* line);

int handle_exit_command(job_info* job, list_t* bg_job_list);

void handle_cd_command(job_info* job);

void handle_estatus_command(job_info* job, int last_child_status);

void handle_bglist_command(job_info* job, list_t* bg_job_list);

int compare_bgentry(const void* a, const void* b);

bgentry_t* find_bg_job_by_pid(list_t* bg_job_list, pid_t pid);

void remove_process_from_list(list_t* bg_job_list, pid_t pid);

void reap_terminated_children(list_t* bg_job_list , int* child_terminated, int* last_child_status );

void handle_fg_command(job_info* job, list_t* bg_job_list);

void handle_bg_process(job_info* job, list_t* bg_job_list, pid_t pid);

void handle_fg_process(job_info* job, list_t* bg_job_list, int* last_child_status, 	pid_t pid);

void execute_child_process(job_info* job);
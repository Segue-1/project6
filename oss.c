#include <stdio.h>
#include <sys/wait.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <locale.h>
#include <sys/time.h>
#include <time.h>
#include <sys/queue.h>
#include <math.h>
#include <errno.h>

#include "global_constants.h"
#include "helpers.h"
#include "shared_memory.h"
#include "message_queue.h"
#include "queue.h"
#include "memory.h"

void wait_for_child();
void sig_handler_getter();
void sigint_handler(int sig);
void sigalrm_handler(int sig);
void clean_then_exit();
void forker(char** execv_arr, unsigned int pid);
struct clock get_time_for_next_process(struct clock sysclock);
unsigned int nano_sec_getter();
int available_pid_getter();
struct message message_parser(char* mtext);
void blocked_q_outputter();
float percentage_getter(int numerator, int denominator);
void childpid_arr_initializer();
void log_file_setup(char* filename);
struct Main_mem main_mem_getter();
void exit_reason_outputter(int proc_cnt);
int time_request_getter(char* request_type);
bool check_for_page_fault(int frame_number);
void proc_terminator(int pid);
bool check_if_unblocked(int pid, struct clock time_unblocked);
struct BlockedInfo blocked_information_getter();
int add_page_to_main_mem(struct Main_mem* main_mem, int page_number);

#define FIFTEEN_MILLION 15000000

// Globals used in signal handler
int maximum_processes_running;
int sim_clk_id, page_table_id, mem_message_box_id, out_message_box_id;
int* page_table;
struct clock* system_clk;
int clean_flg = 0;
pid_t* child_pids;
FILE* file_ptr;

struct message {
    int pid;
    char txt[6];
    int page;
};

struct BlockedInfo {
    int page_number;
    struct clock time_unblocked;
    char type_of_request[6];
};

int main (int argc, char* argv[]) {
    // Maximum time for OSS to run for in seconds
    const unsigned int ELAPSED_RUNTIME = 2;
       
    // Initialize timer for sigalrm
    init_timer(MAXIMUM_RUNTIME);                     
    sig_handler_getter();
    
    // Commas for big numbers      
    setlocale(LC_NUMERIC, "");
    srand(time(NULL) ^ getpid());
    
    // For tracking how long program has ran
    struct timeval tv_start, tv_stop;           
    gettimeofday(&tv_start, NULL);

    // Set maximum number of running processes to command line input
    maximum_processes_running = command_line_argument_getter(argc, argv);
    
    // Keep track of time program has been running in seconds
    int elapsed_seconds = 0;           

    // Used throughout main  loop
    int i, pid = 0, message_num;

    // Output buffer
    char cbuff[255];

    // Keep track of processes
    int process_count = 0, total_processes = 0;

    // Stores time for scheduling new process                   
    struct clock fork_time = clk_getter();

    // For checking number of messages in msg box    
    struct msqid_ds msgq_ds;

    // Main memory simulation                     
    struct Main_mem main_memory = main_mem_getter();

    // For keeping track of statistics
    struct Mem_statistics memory_stats = mem_stats_getter();   
    int frame_num, time_requested, page_num;
    struct BlockedInfo blocked_information_array[maximum_processes_running];
    for (i = 0; i < maximum_processes_running; i++) {
        blocked_information_array[i].time_unblocked = clk_getter();
        blocked_information_array[i].page_number = 0;
    }
    struct BlockedInfo blocked_info_x;
    struct clock time_unblocked;
    struct clock clk_subtraction;
    struct Queue blkd_q;
    init_queue(&blkd_q);

    // Execv array for passing data to child processes
    char* execv_arr[EXECV_SIZE];                
    execv_arr[0] = "./user";
    execv_arr[EXECV_SIZE - 1] = NULL;
    
    // Initialize shared memory below

    // Logical clock
    sim_clk_id = shared_mem_getter();
    system_clk = (struct clock*) shared_mem_attacher(sim_clk_id, 0);
    clk_resetter(system_clk);
    
    // Page Table
    page_table_id = shared_mem_getter();
    page_table = (int*) shared_mem_attacher(page_table_id, 0);
    get_page_table(page_table, maximum_processes_running);

    // Message box is for user processes.  Allows them to request to read or write to memory
    mem_message_box_id = message_q_getter();
    struct msgbuf mem_msg_box;

    out_message_box_id = message_q_getter();
    struct msgbuf out_msg_box;
    
    struct message msg;
    
    // Setup childpid array
    childpid_arr_initializer();

    // Setup log file to write to
    log_file_setup("./oss.log");
    
    // Gets the time which first proc is forked
    fork_time = get_time_for_next_process(*system_clk);

    // Current time gets incremented so that a new user process can be forked    
    *system_clk = fork_time;

    // Primary loop
    while ( elapsed_seconds < ELAPSED_RUNTIME && process_count < 100) {

        // See if it's time for forking another user process
        if (clk_comparison(*system_clk, fork_time) >= 0 && process_count < maximum_processes_running) {
            // If so, then fork
            pid = available_pid_getter();
            
            forker(execv_arr, pid);
            process_count++;
            total_processes++;
            
            sprintf(cbuff, "OSS: Generating P%d at time %ld:%'ld\n",
                pid, system_clk->seconds, system_clk->nanoseconds);
            outputter(cbuff, file_ptr);

            fork_time = get_time_for_next_process(*system_clk);
        }

        if (!empty(&blkd_q)) {

            // If blocked queue isn't empty, check it
            pid = peek(&blkd_q);

	    // Get the blocked information from blocked information structure array
            blocked_info_x = blocked_information_array[pid];
            time_unblocked = blocked_info_x.time_unblocked;

            if (check_if_unblocked(pid, time_unblocked)) {
                // Unblocked process
                page_num = blocked_info_x.page_number;

                sprintf(cbuff, "\nOSS: 15ms have passed. Unblocking P%d and granting %s access on page %d at time %ld:%'ld.\n",
                    pid, blocked_info_x.type_of_request, page_num, system_clk->seconds, system_clk->nanoseconds);
                outputter(cbuff, file_ptr);

                // Put page into main memory
                frame_num = add_page_to_main_mem(&main_memory, page_num);

                // Put frame number into page table
                put_frame_in_page_table(frame_num, page_table, pid);

                // Second chance bit
                main_memory.second_chance[frame_num] = 1;    

                // Frame is set dirty if there is write
                if (strcmp(blocked_info_x.type_of_request, "WRITE") == 0) {
                    main_memory.dirty[frame_num] = 1;
                }

                // Take out of block queue
                dequeue(&blkd_q);

                // Send message
                message_sender(out_message_box_id, &out_msg_box, pid);

                // Update memory_stats to keep track of statistics
                memory_stats.num_memory_accesses++;
    
            }

            // Check if every process has been blocked
            if (count(&blkd_q) == maximum_processes_running) {
                
                clk_subtraction = clk_subtracter(time_unblocked, *system_clk);
                
                // Increment system_clk so that a process is unblocked 
                *system_clk = time_unblocked;
                
                sprintf(cbuff, "\nOSS: All processes blocked because of page faults. Incrementing clock %ld:%'ld to unblock 1 process at time %ld:%'ld.\n\n",
                    clk_subtraction.seconds, clk_subtraction.nanoseconds, system_clk->seconds, system_clk->nanoseconds);
                outputter(cbuff, file_ptr);
            }
        }
        
        // Get the number of messages
        msgctl(mem_message_box_id, IPC_STAT, &msgq_ds);
        message_num = msgq_ds.msg_qnum;

        while (message_num-- > 0) {
            message_receiver(mem_message_box_id, &mem_msg_box, 0);
            msg = message_parser(mem_msg_box.mtext);
            
            pid = msg.pid;
            time_requested = time_request_getter(msg.txt);

            if (strcmp(msg.txt, "TERM") != 0) {

		// There is a process reqeusting to read or write memory
                if (!check_valid_page_num(pid, msg.page)) {

                    // There is a segmentation fault
                    sprintf(cbuff, "\nOSS: P%d seg faulted and will be terminated at time %ld:%'ld.\n\n",
                        pid, system_clk->seconds, system_clk->nanoseconds);
                    outputter(cbuff, file_ptr);
                    
                    // Terminate process
                    proc_terminator(pid);

		    // Free up page numbers from main memory and frame numbers from  page table
                    frame_freer(&main_memory, page_table, pid);

                    // Free up child_pids array
                    child_pids[pid] = 0;
                    process_count--;

                    // Keep track of statistics
                    memory_stats.num_seg_faults++;

                    // Output memory map
                    output_main_mem(file_ptr, main_memory);
                }
                else {
                    // Then page number is valid
                    frame_num = main_mem_frame_getter(main_memory.memory, msg.page);
                    
                    if (check_for_page_fault(frame_num)) {

                        // There is page fault, output info
                        sprintf(cbuff, "OSS: P%d requested %s access on page %d and page faulted at time %ld:%'ld.\n     Adding process to blocked queue.\n",
                            pid, msg.txt, msg.page, system_clk->seconds, system_clk->nanoseconds);
                        outputter(cbuff, file_ptr);
                        
                        // Set blocked info in structure
                        blocked_info_x = blocked_information_getter();
                        blocked_info_x.page_number = msg.page;
                        strcpy(blocked_info_x.type_of_request, msg.txt);
                        blocked_info_x.time_unblocked = *system_clk;
                                        
                        // Blocked information is stored into the blocked information array
                        blocked_information_array[pid] = blocked_info_x;
                        
                        // Increment clock by 15ms
                        clk_incrementer(&blocked_info_x.time_unblocked, FIFTEEN_MILLION);   

                        // Put process into blocked queue
                        enqueue(&blkd_q, pid);
                        
                        // Keep track of statistics
                        memory_stats.num_page_faults++;
                        memory_stats.total_mem_access_time += FIFTEEN_MILLION;
                    }
                    else {
                        // Page is already in main mem frame
                         sprintf(cbuff, "OSS: Granting P%d %s access on page %d at time %ld:%'ld\n",
                             pid, msg.txt, msg.page, system_clk->seconds, system_clk->nanoseconds);

                        
                        // Increment clock
                        clk_incrementer(system_clk, time_requested);

			// Keep track of stats
                        memory_stats.total_mem_access_time += time_requested;

                        // Second chance bit set
                        main_memory.second_chance[frame_num] = 1;    

                        // If there is write set as dirty
                        if (strcmp(msg.txt, "WRITE") == 0) {
                            main_memory.dirty[frame_num] = 1;
                        }

                        // Send message
                        message_sender(out_message_box_id, &out_msg_box, pid);

                        // Keep track of stats
                        memory_stats.num_memory_accesses++;
                    }
                }
            }
            else {
                // Terminated process
                sprintf(cbuff, "\nOSS: Acknowledging P%d terminated at time %ld:%'ld\n\n",
                    pid, system_clk->seconds, system_clk->nanoseconds);
                outputter(cbuff, file_ptr);
                
                // Free page numbers and frame numbers
                frame_freer(&main_memory, page_table, pid);
                
                // Child pid array space is freed up
                child_pids[pid] = 0;
                process_count--;

                // Output memory map
                output_main_mem(file_ptr, main_memory);
                
            }
        }

        // Increment clock a little 
        clk_incrementer(system_clk, nano_sec_getter());

        // Calculate elapsed time in seconds
        gettimeofday(&tv_stop, NULL);
        elapsed_seconds = tv_stop.tv_sec - tv_start.tv_sec;
    }

    exit_reason_outputter(process_count);

    // Load memory_stats
    memory_stats.num_seconds = clk_to_secs(*system_clk);
    memory_stats.proc_cnt = total_processes;

    output_stats(file_ptr, memory_stats);

    clean_then_exit();

    return 0;
}

void forker(char** execv_arr, unsigned int pid) {
    if ((child_pids[pid] = fork()) == 0) {
        // Setup if child
        char clock_id[10];
        char pg_tbl_id[10];
        char msgbox_id[10];
        char out_msgbox_id[10];
        char p_id[5];
        
        sprintf(clock_id, "%d", sim_clk_id);
        sprintf(pg_tbl_id, "%d", page_table_id);
        sprintf(msgbox_id, "%d", mem_message_box_id);
        sprintf(out_msgbox_id, "%d", out_message_box_id);
        sprintf(p_id, "%d", pid);
        
        execv_arr[SYSTEM_CLK_ID] = clock_id;
        execv_arr[PAGE_TABLE_ID] = pg_tbl_id;
        execv_arr[MEMORY_MSG_BX_ID] = msgbox_id;
        execv_arr[OUT_MSG_BX_ID] = out_msgbox_id;
        execv_arr[PID_ID] = p_id;

        execvp(execv_arr[0], execv_arr);

        perror("Child failed to execvp the command!");
        exit(1);
    }

    if (child_pids[pid] == -1) {
        perror("Child failed to fork\n");
        exit(1);
    }
}

void wait_for_child() {
    pid_t pid;
    printf("OSS: Waiting for all children to exit\n");
    fprintf(file_ptr, "OSS: Waiting for all children to exit\n");
    
    while ((pid = wait(NULL))) {
        if (pid < 0) {
            if (errno == ECHILD) {
                perror("wait");
                break;
            }
        }
    }
}

void terminate_children() {
    printf("OSS: Sending SIGTERM to all children\n");
    fprintf(file_ptr, "OSS: Sending SIGTERM to all children\n");
    int i;
    for (i = 0; i < maximum_processes_running; i++) {
        if (child_pids[i] == 0) {
            continue;
        }
        proc_terminator(i);
    }
    free(child_pids);
}

void proc_terminator(int pid) {
    if (kill(child_pids[pid], SIGTERM) < 0) {
        if (errno != ESRCH) {
            // Termination failled
            perror("Termination");
        }
    }
}

void sig_handler_getter() {
    struct sigaction act;
    // Sig handler
    act.sa_handler = sigint_handler;

    // Other signals shouldn't be blocked
    sigemptyset(&act.sa_mask);
      
    act.sa_flags = 0;               
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    // Sig handler
    act.sa_handler = sigalrm_handler; 

    // Other signals shouldn't be blocked
    sigemptyset(&act.sa_mask);       
    if (sigaction(SIGALRM, &act, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void sigint_handler(int sig) {
    printf("\nOSS: Caught SIGINT signal %d\n", sig);
    fprintf(file_ptr, "\nOSS: Caught SIGINT signal %d\n", sig);
    if (clean_flg == 0) {
        clean_flg = 1;
        clean_then_exit();
    }
}

void sigalrm_handler(int sig) {
    printf("\nOSS: Caught SIGALRM signal %d\n", sig);
    fprintf(file_ptr, "\nOSS: Caught SIGALRM signal %d\n", sig);
    if (clean_flg == 0) {
        clean_flg = 1;
        clean_then_exit();
    }

}

void clean_then_exit() {
    terminate_children();
    printf("OSS: Removing message queues and shared memory\n");
    fprintf(file_ptr, "OSS: Removing message queues and shared memory\n");
    message_q_remover(mem_message_box_id);
    wait_for_child();
    shared_mem_cleaner(sim_clk_id, system_clk);
    shared_mem_cleaner(page_table_id, page_table);
    fclose(file_ptr);
    exit(0);
}

struct clock get_time_for_next_process(struct clock system_clk) {
    unsigned int ns_before_next_proc = rand() % MAX_NANO_SEC_UNTIL_NEXT_PROC; 
    clk_incrementer(&system_clk, ns_before_next_proc);
    return system_clk;
}

unsigned int nano_sec_getter() {
    return (rand() % 50000) + 10000; // 500 - 5,000 inclusive
}

int available_pid_getter() {
    int pid, i;
    for (i = 0; i < maximum_processes_running; i++) {
        if (child_pids[i] > 0) {
            continue;
        }
        pid = i;
        break;
    }
    return pid;
}

struct message message_parser(char* mtext) {
    // Messages from user process are parsed
    struct message msg;
    char ** msg_info = clean_string(mtext, ",");
    
    msg.pid = atoi(msg_info[0]);
    strcpy(msg.txt, msg_info[1]);
    msg.page = atoi(msg_info[2]);

    free(msg_info);

    return msg;
}


float percentage_getter(int numerator, int denominator) {
    return  (numerator / (float) denominator) * 100;
}

void childpid_arr_initializer() {
    int i;
    child_pids = malloc(sizeof(pid_t) * maximum_processes_running);
    for (i = 0; i < maximum_processes_running; i++) {
        child_pids[i] = 0;
    }
}

void log_file_setup(char* filename) {
    if ((file_ptr = fopen(filename, "w")) == NULL) {
        perror("fopen");
        exit(1);
    }
}

void exit_reason_outputter(int process_count) {
    // Output info before exit
    char reason[100];
    char buffer[100];
    
    if (process_count < 100) {
        sprintf(reason, "because %d seconds have been passed", ELAPSED_RUNTIME);
    }
    else {
        sprintf(reason, "because %d processes have been generated", process_count);
    }

    sprintf(buffer, "OSS: Exiting at time %'ld:%'ld %s\n", 
        system_clk->seconds, system_clk->nanoseconds, reason);
    outputter(buffer, file_ptr);

    sprintf(buffer, "\n");
    outputter(buffer, file_ptr);
}

int time_request_getter(char* request_type) {
    if (strcmp(request_type, "READ") == 0) {
        return 10; // nanoseconds
    }
    else if (strcmp(request_type, "WRITE") == 0) {
        return 20; // nanseconds
    }
    return 0;
}

bool check_for_page_fault(int frame_num) {
    if (frame_num < 0) {
        return 1;
    }
    return 0;
}

bool check_if_unblocked(int pid, struct clock time_unblocked) {
    if (clk_comparison(*system_clk, time_unblocked) >=  0) {
        return 1;
    }
    return 0;
}

struct BlockedInfo blocked_information_getter() {
    struct BlockedInfo binfo = {
        .page_number = 0,
        .time_unblocked = clk_getter(),
        .type_of_request = ""
    };
    return binfo;
}

int add_page_to_main_mem(struct Main_mem* main_mem, int page_num) {
    char buffer[256];
    int free_frame_number = free_frame_num_getter(main_mem->memory);

    if (check_if_main_mem_full(free_frame_number)) {
        // Swap page
        free_frame_number = try_page_replace_again(main_mem);

        sprintf(buffer, "     Main memory is full so swapping page %d in frame %d with page %d\n\n",
            main_mem->memory[free_frame_number], free_frame_number, page_num);
        outputter(buffer, file_ptr);

        if (main_mem->dirty[free_frame_number]) {
            // Swap dirty page
            sprintf(buffer, "     Swapping out a dirty page, so incrementing the clock 15ms to simulate saving contents of the page to disk at time %ld:%'ld.\n\n",
                system_clk->seconds, system_clk->nanoseconds);
            outputter(buffer, file_ptr);

            clk_incrementer(system_clk, FIFTEEN_MILLION);
        }
    }
    else {
        // Page put into main memory
        sprintf(buffer, "     Main memory is not full so putting page %d in frame %d\n\n",
            page_num, free_frame_number);
        outputter(buffer, file_ptr);
    }
    
    main_mem->memory[free_frame_number] = page_num;

    return free_frame_number;
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/time.h>
#include <locale.h>
#include <signal.h>

#include "global_constants.h"
#include "helpers.h"
#include "message_queue.h"
#include "shared_memory.h"
#include "clock.h"
#include "memory.h"

bool to_be_terminated();
bool mem_read();
void sig_handler_add();
void sigterm_handler(int sig);
int rand_valid_page_num_getter(int page_table_start);
int rand_invalid_page_num_getter(int page_table_start);
int page_num_getter(int page_table_start);
void termination_notif_sender(int mem_msg_box_id, int pid);
int number_of_memory_references_getter();

void reference_type_getter(char* ref_type);

// Percent chance to terminate (1-100), don't put 0
const unsigned int CHANCE_TERMINATE = 1;

// Chance to seg fault out of 10,000
const unsigned int CHANCE_SEG_FAULT = 1; 
const unsigned int CHANCE_READ = 97;

int main (int argc, char *argv[]) {
    sig_handler_add();
    srand(time(NULL) ^ getpid());

    // Store the shared memory id of clock, page table, message box, and pid
    int system_clk_id = atoi(argv[SYSTEM_CLK_ID]);
    int page_table_id = atoi(argv[PAGE_TABLE_ID]);
    int memory_message_box_id = atoi(argv[MEMORY_MSG_BX_ID]);
    int out_message_box_id = atoi(argv[OUT_MSG_BX_ID]);
    int pid = atoi(argv[PID_ID]);

    // Shared memory is attached 
    struct clock* system_clk = shared_mem_attacher(system_clk_id, 1);
    int* page_tbl = shared_mem_attacher(page_table_id, 0);

    // Set variables
    int page_table_start = pid * PROCESS_PAGES;
    int ref_before_term = number_of_memory_references_getter();
    int number_of_requests = 0;
    int page_num;
    struct msgbuf mem_msg_box, out_msg_box;
    char ref_type[6];

    while (1) {
        // See if memory is written or read
        reference_type_getter(ref_type);
        
        page_num = page_num_getter(page_table_start);

        // Make message for OSS
        sprintf(mem_msg_box.mtext, "%d,%s,%d", pid, ref_type, page_num);

        // Message is sent to OSS
        message_sender(memory_message_box_id, &mem_msg_box, pid);
        
        // Wait for OSS to grant request since blocking has been received
        message_receiver(out_message_box_id, &out_msg_box, pid);
        
        if (number_of_requests == ref_before_term) {

            // Check if process will be terminated
            if (to_be_terminated()) {
                termination_notif_sender(memory_message_box_id, pid);
                break;
            }
            ref_before_term += number_of_memory_references_getter();
        }

        number_of_requests++;
    }

    return 0;  
}

bool will_seg_fault() {
    return event_happened_per_thousand(CHANCE_SEG_FAULT);
}

bool to_be_terminated() {
    return event_happened(CHANCE_TERMINATE);
}

bool mem_read() {
    return event_happened(CHANCE_READ);
}

void sig_handler_add() {
    struct sigaction act;
    // Sig handler
    act.sa_handler = sigterm_handler; 
    // Other signals shouldn't be blocked
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;               
    if (sigaction(SIGTERM, &act, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void sigterm_handler(int signal) {
    _exit(0);
}

int rand_valid_page_num_getter(int page_table_start) {
    int random_offset = rand() % PROCESS_PAGES;
    return page_table_start + random_offset;
}

int rand_invalid_page_num_getter(int page_table_start) {
    int random_offset = rand() % PROCESS_PAGES;
    return page_table_start - random_offset;
}

int page_num_getter(int page_table_start) {
    if (will_seg_fault()) {
        return rand_invalid_page_num_getter(page_table_start);
    }
    else {
        return rand_valid_page_num_getter(page_table_start);
    }
}

void reference_type_getter(char* ref_type) {
    if (mem_read()) {
        sprintf(ref_type, "READ");
    } 
    else { 
        // Write to memory
        sprintf(ref_type, "WRITE");
    } 
}

void termination_notif_sender(int memory_message_box_id, int pid) {
    struct msgbuf mem_msg_box;
    sprintf(mem_msg_box.mtext, "%d,TERM,0", pid);
    message_sender(memory_message_box_id, &mem_msg_box, pid); 
}

int number_of_memory_references_getter() {
    int base = 900;
    int random_offset = rand() % 200;
    return base + random_offset;
}
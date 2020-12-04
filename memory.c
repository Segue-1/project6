#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "helpers.h"

struct Main_mem main_mem_getter() {
    struct Main_mem mainmemory;
    int i;
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        mainmemory.memory[i] = 0;
        mainmemory.second_chance[i] = 0;
        mainmemory.dirty[i] = 0;
    }
    return mainmemory;
}

void get_page_table(int* page_table, int maximum_processes_running) {
    int i, total_pages = page_total_getter(maximum_processes_running);
    for (i = 0; i < total_pages; i++) {
        page_table[i] = 0;
    }
}

struct Mem_statistics mem_stats_getter() {
    struct Mem_statistics stats = {
        .num_memory_accesses = 0,
        .num_seconds = 0,
        .proc_cnt = 0,
        .num_page_faults = 0,
        .num_seg_faults = 0,
        .total_mem_access_time = 0
    };

    return stats;
}


void output_stats(FILE* f_ptr, struct Mem_statistics stats) {
    char buffer[1000];

    // Makes calculations
    float mem_access_per_ms = stats.num_memory_accesses / (stats.num_seconds * 1000);
    float page_fault_per_access = stats.num_page_faults / (float) stats.num_memory_accesses;
    float avg_mem_access_speed = stats.total_mem_access_time / (float) stats.num_memory_accesses;
    avg_mem_access_speed /= ONE_MILLION; // Convert from nanoseconds to milliseconds
    float throughput = stats.proc_cnt / stats.num_seconds;

    // Output
    sprintf(buffer, "Statistics\n");
    sprintf(buffer + strlen(buffer), "  %-28s: %'d\n", "Memory Accesses", stats.num_memory_accesses);
    sprintf(buffer + strlen(buffer), "  %-28s: %.2f\n", "Memory Accesses/Millisecond", mem_access_per_ms);
    sprintf(buffer + strlen(buffer), "  %-28s: %.2f\n", "Page Faults/Memory Access", page_fault_per_access);
    sprintf(buffer + strlen(buffer), "  %-28s: %'.2f ms\n", "Avg Memory Access Speed", avg_mem_access_speed); 
    sprintf(buffer + strlen(buffer), "  %-28s: %'d\n", "Segmentation Faults", stats.num_seg_faults);
    sprintf(buffer + strlen(buffer), "  %-28s: %.2f processes/sec\n", "Throughput", throughput);
    sprintf(buffer + strlen(buffer), "\n");
    
    outputter(buffer, f_ptr);
}

int page_total_getter(int maximum_processes_running) {
    return maximum_processes_running * PROCESS_PAGES;
}

int start_index_getter(int pid) {
    return pid * PROCESS_PAGES;
}

int end_index_getter(int start_index) {
    return start_index + PROCESS_PAGES;
}

bool check_valid_page_num(int pid, int page_num) {

    // Checks if page_num is valid with pid
    int start_index = start_index_getter(pid);
    int end_index = end_index_getter(start_index);
    if (page_num < start_index || page_num > end_index) {
        return 0;
    }
    return 1;
}

int main_mem_frame_getter(int* main_mem, int page_num) {

    // Returns page frame number if valid
    int i;
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (page_num != main_mem[i]) {
            continue;
        }
        return i;
    }
    return -1;
}

void frame_freer(struct Main_mem* main_memory, int* page_table, int pid) {
    int start_index = start_index_getter(pid);
    int end_index = end_index_getter(start_index);
    int i, frame_number;
    for (i = start_index; i < end_index; i++) {
        if (page_table[i] == 0) {
            // No entry in page table
            continue;
        }
        frame_number = page_table[i];
        main_memory->memory[frame_number] = 0;
        main_memory->dirty[frame_number] = 0;
        main_memory->second_chance[frame_number] = 0;
        page_table[i] = 0;
    }
    return;
}

int free_frame_num_getter(int* main_memory) {
    int i;
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (main_memory[i] != 0) {
            continue;
        }
        return i;
    }
    // If there isn't a free frame return -1
    return -1; 
}

bool check_if_main_mem_full(int free_frame_num) {
    if (free_frame_num < 0) {
        return 1;
    }
    return 0;
}

int try_page_replace_again(struct Main_mem* main_memory) {

    // Assuming there is a frame stored in every page
    int i, frame_number = -1;
    
    i = main_memory->second_chance_ptr;
    
    while (frame_number < 0) {

        // Reposition index so that if we're at end it goes back to start, wrap around
        if (i == MAIN_MEMORY_SZE) {
            i = 0;
        }
        
        if (main_memory->second_chance[i]) {

            // Second chance bit is set therefor give it second chance
            main_memory->second_chance[i] = 0;
        }
        else {
            frame_number = i;
        }

        i++;
    }

    main_memory->second_chance_ptr = i;

    return frame_number; 
}

void put_frame_in_page_table(int frame_num, int* page_table, int pid) {
    int start_index = start_index_getter(pid);
    int end_index = end_index_getter(start_index);
    int i;
    for (i = start_index; i < end_index; i++) {
        if (page_table[i] != 0) {
            continue;
        }
        // Nothing in page table
        page_table[i] = frame_num;
        
        break;
    }
}

void output_frames(int* page_table, int pid) {
    int start_index = start_index_getter(pid);
    int end_index = end_index_getter(start_index);
    int i;
    printf("pages = %d\n", end_index - start_index);
    for (i = start_index; i < end_index; i++) {
        if (page_table[i] != 0) {
            printf("%3d ", page_table[i]);
        }
        else {
            printf(". ");
        }
    }
    printf("\n");
}

void output_main_mem(FILE* fp, struct Main_mem main_memory) {
    char buffer[1000];
    int i;
    sprintf(buffer, "Main Memory\n  ");
    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (main_memory.memory[i] == 0) {
            sprintf(buffer + strlen(buffer), ".");
        }
        else if (main_memory.dirty[i]) {
            sprintf(buffer + strlen(buffer), "D");
        }
        else {
            // Frame is occupied
            sprintf(buffer + strlen(buffer), "U");
        }
    }

    sprintf(buffer + strlen(buffer), "\n  ");

    for (i = 0; i < MAIN_MEMORY_SZE; i++) {
        if (main_memory.memory[i] == 0) {
            sprintf(buffer + strlen(buffer), ".");
        }
        else if (main_memory.second_chance[i]) {
            sprintf(buffer + strlen(buffer), "1");
        }
        else {
            // Second chance bit is not set
            sprintf(buffer + strlen(buffer), "0");
        }
    }

    sprintf(buffer + strlen(buffer), "\n");

    outputter(buffer, fp);
}
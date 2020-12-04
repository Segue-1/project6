#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include <stdio.h>
#include "clock.h"

#define MAIN_MEMORY_SZE 256
#define PROCESS_PAGES 32
#define ONE_MILLION 1000000

struct Main_mem {
    int memory[MAIN_MEMORY_SZE];
    bool second_chance[MAIN_MEMORY_SZE];
    bool dirty[MAIN_MEMORY_SZE];
    int second_chance_ptr;
};

struct Mem_statistics {
    int num_memory_accesses;
    int num_page_faults;
    int num_seg_faults;
    int proc_cnt;
    long double num_seconds;
    unsigned long total_mem_access_time;
};

struct Main_mem main_mem_getter();
void get_page_table(int* page_table, int maximum_processes_running);
void output_stats(FILE* fp, struct Mem_statistics stats);
struct Mem_statistics mem_stats_getter();
int page_total_getter(int maximum_processes_running);
bool check_valid_page_num(int pid, int page_num);
int main_mem_frame_getter(int* mainmemory, int page_number);
int start_index_getter(int pid);
int end_index_getter(int start_index);
void frame_freer(struct Main_mem* mainmemory, int* page_table, int pid);
int free_frame_num_getter(int* mainmemory);
bool check_if_main_mem_full(int free_frame_num);
int try_page_replace_again(struct Main_mem* mainmemory);
void put_frame_in_page_table(int frame_num, int* page_table, int pid);
void output_main_mem(FILE* fp, struct Main_mem mainmemory);
void output_frames(int* page_table, int pid);

#endif

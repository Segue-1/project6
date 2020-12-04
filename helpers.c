#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "helpers.h"

char** clean_string(char* s, char* delim) {
    char** strings = malloc(10 * sizeof(char*));
    char* substr;

    substr = strtok(s, delim);

    int i = 0;
    while (substr != NULL)
    {
        strings[i] = substr;
        substr = strtok(NULL, delim);
        i++;
    }

    return strings;

}

char* time_stamp_getter() {
    char* timestamp = malloc(sizeof(char)*10);
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    sprintf(timestamp, "%d:%d:%d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    return timestamp;
}

int command_line_argument_getter(int argc, char* argv[]) {
    int option;
    int num_child_procs = 0;
    while ((option = getopt (argc, argv, "hn:")) != -1)
    switch (option) {
        case 'h':
            output_usage();
            break;
        case 'n':
            num_child_procs = atoi(optarg);
            break;
        default:
            output_usage();
    }

    if (num_child_procs == 0) {
        // Default to 8
        num_child_procs = 8;
    }

    if (num_child_procs > 18) {
        // Set max to 18
        num_child_procs = 18;
    }
    
    printf("num child procs set to %d\n", num_child_procs);
    return num_child_procs;
}

void output_usage() {
    fprintf(stderr, "Usage: ./oss [-n maximum concurrent children processes]\n");
    exit(0);
}

void init_timer(int length) {
    struct itimerval value;
    value.it_interval.tv_sec = length;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    if (setitimer(ITIMER_REAL, &value, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }
}

bool event_happened(unsigned int chance_percentage) {
    unsigned int percent = (rand() % 100) + 1;
    if (percent <= chance_percentage) {
        return 1;
    }
    else {
        return 0;
    }
}

bool event_happened_per_thousand(unsigned int chance) {
    unsigned int number = (rand() % 10000) + 1;
    if (number <= chance) {
        return 1;
    }
    else {
        return 0;
    }
}

unsigned int** make_arr(int a, int b) {
    // Makes a rows by b column matrix
    unsigned int* values = calloc(a * b, sizeof(unsigned int));
    unsigned int** rows = malloc(a * sizeof(unsigned int*));
    int i;
    for (i = 0; i < a; i++)
    {
        rows[i] = values + (i * b);
    }
    return rows;
}

void delete_arr(unsigned int** arr) {
    free(*arr);
    free(arr);
}

void outputter(char* s, FILE* file_ptr) {
    fputs(s, stdout);
    fputs(s, file_ptr);
}
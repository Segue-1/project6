#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>

char** clean_string(char* s, char* delim);
char* time_stamp_getter();
void output_usage();
int command_line_argument_getter(int argc, char* argv[]);
void init_timer(int duration);
bool event_happened(unsigned int pct_chance);
unsigned int** make_arr(int a, int b);
void delete_arr(unsigned int** arr);
void outputter(char* s, FILE* file_ptr);
bool event_happened_per_thousand(unsigned int chance);

#endif

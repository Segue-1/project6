#include "clock.h"
#include <stdio.h>

#define ONE_BILLION 1000000000

void clk_incrementer(struct clock* clk, int incr) {
    clk->nanoseconds += incr;
    if (clk->nanoseconds >= ONE_BILLION) {
        clk->seconds += 1;
        clk->nanoseconds -= ONE_BILLION;
    }
}

struct clock clk_adder(struct clock clk_1, struct clock clk_2) {
    struct clock out = {
        .seconds = 0,
        .nanoseconds = 0
    };
    out.seconds = clk_1.seconds + clk_2.seconds;
    clk_incrementer(&out, clk_1.nanoseconds + clk_2.nanoseconds);
    return out;
}

int clk_comparison(struct clock clk_1, struct clock clk_2) {
    if (clk_1.seconds > clk_2.seconds) {
        return 1;
    }
    if ((clk_1.seconds == clk_2.seconds) && (clk_1.nanoseconds > clk_2.nanoseconds)) {
        return 1;
    }
    if ((clk_1.seconds == clk_2.seconds) && (clk_1.nanoseconds == clk_2.nanoseconds)) {
        return 0;
    }
    return -1;
}

long double clk_to_secs(struct clock clk) {
    long double seconds = clk.seconds;
    long double nanoseconds = (long double)clk.nanoseconds / ONE_BILLION; 
    seconds += nanoseconds;
    return seconds;
}

struct clock secs_to_clk(long double secs) {
    struct clock clk = { .seconds = (int)secs };
    secs -= clk.seconds;
    clk.nanoseconds = secs * ONE_BILLION;
    return clk;
}

struct clock get_time_average(struct clock clk, int denominator) {
    long double seconds = clk_to_secs(clk);
    long double avg_seconds = seconds / denominator;
    return secs_to_clk(avg_seconds);
}

struct clock clk_subtracter(struct clock clk_1, struct clock clk_2) {
    long double seconds1 = clk_to_secs(clk_1);
    long double seconds2 = clk_to_secs(clk_2);
    long double result = seconds1 - seconds2;
    return secs_to_clk(result);
}

void clk_outputter(char* name, struct clock clk) {
    printf("%-15s: %'ld:%'ld\n", name, clk.seconds, clk.nanoseconds);
}

struct clock nanosecs_to_clk(int nanosecs) {
    // Assumes nanoseconds is less than 2 billion
    struct clock clk = { 
        .seconds = 0, 
        .nanoseconds = 0 
    };

    if (nanosecs >= ONE_BILLION) {
        nanosecs -= ONE_BILLION;
        clk.seconds = 1;
    }

    clk.nanoseconds = nanosecs;
    
    return clk;
}

struct clock clk_getter() {
    struct clock out = {
        .seconds = 0,
        .nanoseconds = 0
    };
    return out;
}

void clk_resetter(struct clock* clk) {
    clk->seconds = 0;
    clk->nanoseconds = 0;
}
#ifndef CLOCK_H
#define CLOCK_H

struct clock {
    unsigned long seconds;
    unsigned long nanoseconds;
};

void clk_incrementer(struct clock* clk, int incr);
struct clock clk_adder(struct clock clk_1, struct clock clk_2);
int clk_comparison(struct clock clk_1, struct clock clk_2);
long double clk_to_secs(struct clock clk);
struct clock secs_to_clk(long double secs);
struct clock get_time_average(struct clock clk, int denominator);
struct clock clk_subtracter(struct clock clk_1, struct clock clk_2);
void clk_outputter(char* name, struct clock clk);
struct clock nanosecs_to_clk(int nanosecs);
struct clock clk_getter();
void clk_resetter(struct clock* clk);

#endif

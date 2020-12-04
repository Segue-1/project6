#ifndef GLOBAL_CONSTANTS_H
#define GLOBAL_CONSTANTS_H

#define FIVE_HUNDRED_MILLION 500000000

const unsigned int EXECV_SIZE = 7;
const unsigned int SYSTEM_CLK_ID = 1;
const unsigned int PAGE_TABLE_ID = 2;
const unsigned int MEMORY_MSG_BX_ID = 3;
const unsigned int OUT_MSG_BX_ID = 4;
const unsigned int PID_ID = 5;

// ELAPSED_RUNTIME and MAXIMUM_RUNTIME are in seconds
const unsigned int ELAPSED_RUNTIME = 2; 
const unsigned int MAXIMUM_RUNTIME = 20;  
const unsigned int MAXIMUM_PROCESS_COUNT = 18;

const unsigned int MAX_NANO_SEC_UNTIL_NEXT_PROC = FIVE_HUNDRED_MILLION; 
#endif

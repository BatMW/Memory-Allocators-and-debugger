#ifndef CLIBS_ERROR_SIGNALS
#define CLIBS_ERROR_SIGNALS
#include "stdio.h"
#include "signal.h"
#include "stdlib.h"
#include "execinfo.h"

/*
Use flags: -std=c++20 -Wall -Wextra -g -pedantic -fno-omit-frame-pointer -rdynamic -Og -DDEBUG

*/

#ifdef DEBUG

void ERR_signal_handler(int sig) {
    printf("Error: signal %d received.\n", sig);
    
    // Print backtrace
    const int max_frames = 64;
    void* frames[max_frames];
    int num_frames = backtrace(frames, max_frames);
    char** symbols = backtrace_symbols(frames, num_frames);
    if (symbols != NULL) {
        printf("Backtrace:\n");
        for (int i = 0; i < num_frames; ++i) {
            printf("%s",symbols[i]);
        }
        free(symbols);
    }

    exit(EXIT_FAILURE);
}

void ERR_init_backtrace(){
    // Register signal handlers
   signal(SIGSEGV, ERR_signal_handler); // Segmentation fault
   signal(SIGABRT, ERR_signal_handler); // Abort, assert()
   signal(SIGFPE, ERR_signal_handler);  // Floating point exception
   signal(SIGILL, ERR_signal_handler);  // Illegal instruction
   signal(SIGBUS, ERR_signal_handler);  // Bus Error
   signal(SIGSYS, ERR_signal_handler); // Bad system call
    
    //signal(SIGTERM, signal_handler); //External call to end process
   signal(SIGINT, ERR_signal_handler); //ctrl-c etc. Could be useful if the program gets stuck maybe?
    //signal(SIGQUIT, signal_handler); 
    

}

#endif //DEBUG
#endif //CLIBS_ERROR_SIGNALS

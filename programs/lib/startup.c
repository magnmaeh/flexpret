/**
 * FlexPRET's startup code in C
 * 
 * Authors:
 * - Shaokai Lin
 * - Samuel Berkun
 */

#include <unistd.h>      // Declares _exit() with definition in syscalls.c.
#include <stdint.h>
#include <stdbool.h>
#include <flexpret_io.h>
#include <flexpret_exceptions.h>
#include <flexpret_lock.h>
#include <flexpret_thread.h>
#ifndef BOOTLOADER 
#include "tinyalloc/tinyalloc.h" // Only include tinyalloc in applications, not bootloader
#endif

#define DSPM_LIMIT          ((void*)0x20004000) // 0x4000 = 16KB
#define TA_MAX_HEAP_BLOCK   1000
#define TA_ALIGNMENT        4

extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t end;

static bool     __ready__;
extern uint32_t num_threads_exited;

//prototype of main
int main(void);

/**
 * Allocate a requested memory and return a pointer to it.
 */
#ifndef BOOTLOADER
void *malloc(size_t size) {
    return ta_alloc(size);
}

/**
 * Allocate a requested memory, initial the memory to 0,
 * and return a pointer to it.
 */
void *calloc(size_t nitems, size_t size) {
    return ta_calloc(nitems, size);
}

/**
 * resize the memory block pointed to by ptr
 * that was previously allocated with a call
 * to malloc or calloc.
 */
void *realloc(void *ptr, size_t size) {
    return ta_realloc(ptr, size);
}

/**
 * Deallocate the memory previously allocated by a call to calloc, malloc, or realloc.
 */
void free(void *ptr) {
    ta_free(ptr);
}
#endif // BOOTLOADER

/**
 * Initialize initialized global variables, set uninitialized global variables
 * to zero, configure tinyalloc, and jump to main.
 */
#ifdef BOOTLOADER
void Reset_Handler() {
    // Get hartid
    uint32_t hartid = read_hartid();
    _fp_print(hartid);
    // Only thread 0 performs the setup,
    // the other threads busy wait until ready.
    if (hartid == 0) {
        // Copy .data section into the RAM
        uint32_t size   = &__data_end__ - &__data_start__;
        uint32_t *pDst  = (uint32_t*)&__data_start__;       // RAM
        uint32_t *pSrc  = (uint32_t*)&__etext;              // ROM

        for (uint32_t i = 0; i < size; i++) {
            *pDst++ = *pSrc++;
        }
    }

    // Setup exception handling
    setup_exceptions();

    // Jump to main (which should be the bootloader)
    main();
    
    // Exit the program.
    _exit(0);
    
    // Infinite loop
    while (1);
}
#else
void Reset_Handler() {
    // Get hartid
    uint32_t hartid = read_hartid();

    _fp_print(hartid);
    // Only thread 0 performs the setup,
    // the other threads busy wait until ready.
    if (hartid == 0) {
        // Copy .data section into the RAM
        uint32_t size   = &__data_end__ - &__data_start__;
        uint32_t *pDst  = (uint32_t*)&__data_start__;       // RAM
        uint32_t *pSrc  = (uint32_t*)&__etext;              // ROM

        for (uint32_t i = 0; i < size; i++) {
            *pDst++ = *pSrc++;
        }

    // Initialize tinyalloc.
    ta_init( 
        &end, // start of the heap space
        DSPM_LIMIT,
        TA_MAX_HEAP_BLOCK, 
        16, // split_thresh: 16 bytes (Only used when reusing blocks.)
        TA_ALIGNMENT
    );

    // Setup exception handling
    setup_exceptions();

    // Signal ready.
    hwlock_acquire();
    __ready__ = true;
    hwlock_release();
    } else {
        // Wait for thread 0 to finish setup.
        // FIXME: Use delay until (DU)
        // for precise synchronization.
        while (!__ready__);
    }

    // Call main().
    if (hartid == 0) {
        main();
    } else {
        worker_main();
    }

    // Exit by calling the _exit() syscall.
    if (hartid == 0) {
        // Mark the main thread as exited.
        hwlock_acquire();
        num_threads_exited += 1;
        hwlock_release();

        // Wait for all hardware worker threads to exit.
        while (num_threads_exited < NUM_THREADS);

        // Exit the program.
        _exit(0);
    } else {
        while (1);
    }
}
#endif

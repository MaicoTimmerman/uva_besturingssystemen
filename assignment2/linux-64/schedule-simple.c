/* This file contains a bare bones skeleton for the scheduler function
 * for the second assignment for the OSN course of the 2005 fall
 * semester.
 *
 * Author
 * Joe Kruger & Maico Timmerman
 * Section Computational Science
 * Universiteit van Amsterdam
 *
 * Date:
 * October 23, 2003
 * Modified:
 * September 29, 2005
 */

#include <stdio.h>
#include <stdlib.h>

#include "schedule-simple.h"
#include "mem_alloc.h"


/* This variable will simulate the allocatable memory */
static long memory[MEM_SIZE];

/* TODO:The actual CPU scheduler is implemented here */
static void CPU_scheduler() {
    /* Insert the code for a MLFbQ scheduler here */
}

/* The high-level memory allocation scheduler is implemented here */
static void GiveMemory() {

    int index;
    pcb* proc;
    pcb* stub;

    proc = new_proc;

    while (proc) {
        /*
         * Search for a new process that should be given memory.
         * Insert search code and criteria here.
         * Attempt to allocate as follows:
         */
        index = mem_get(proc->MEM_need);

        /* Allocation succeeded, now put in ready queue. */
        if (index >= 0) {

            /* Set the given memory */
            proc->MEM_base = index;

            /* Header of queue is one further */
            new_proc = proc->next;

            /* Enqueue the process in the ready queue, automatically removing
             * it from all the other queues by resetting prev and next pointers. */
            stub = ready_proc;
            if (stub) {
                /* If queue has elements, walk to the end. */
                while (stub->next)
                    stub = stub->next;
                stub->next = proc;
                proc->prev = stub;
                proc->next = NULL;
            }
            else {
                /* Only process in the new queue */
                ready_proc = proc;
                proc->next = NULL;
                proc->prev = NULL;

            }

            /* Set the next try. */
            proc = new_proc;
        } else {
            break;
        }
    }
}

/* Here we reclaim the memory of a process after it has finished */
static void ReclaimMemory() {
    pcb *proc;

    proc = defunct_proc;
    while (proc) {
        /* Free your own administrative structure if it exists */
        if (proc->your_admin) {
            free(proc->your_admin);
        }

        /* Free the simulated allocated memory */
        mem_free(proc->MEM_base);
        proc->MEM_base = -1;

        /* Call the function that cleans up the simulated process */
        rm_process(&proc);

        /* See if there are more processes to be removed */
        proc = defunct_proc;
    }
}

/* You may want to have the last word... */
static void my_finale() {
    /* TODO:Your very own code goes here */
}

/* The main scheduling routine */
void schedule(event_type event) {
    static int first = 1;

    /* Set initial variables */
    if (first) {
        mem_init(memory);
        finale = my_finale;
        first = 0;
    }

    switch (event) {
        case NewProcess_event:
            GiveMemory();
            break;
        case Time_event:
        case IO_event:
            CPU_scheduler();
            break;
        case Ready_event:
            break;
        case Finish_event:
            ReclaimMemory();
            GiveMemory();
            CPU_scheduler();
            break;
        default:
            printf("I cannot handle event nr. %d\n", event);
            break;
    }
}

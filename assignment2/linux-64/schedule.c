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

#include "schedule.h"
#include "mem_alloc.h"


/* This variable will simulate the allocatable memory */
static long memory[MEM_SIZE];
static int n_memory_alloc_tries;
static int slice_length;

/* TODO:The actual CPU scheduler is implemented here */
static void CPU_scheduler() {
    /* Insert the code for a MLFbQ scheduler here */
}

/* The high-level memory allocation scheduler is implemented here */
static void GiveMemory() {

    int index;
    int n_tries;
    pcb* proc;
    pcb* stub;

    n_tries = 0;
    proc = new_proc;

    while (proc && (n_tries < n_memory_alloc_tries)) {
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

            /* If n is 0 then the item was the first from the queue
             * Else it is in the middle, and pointers need to be moved */
            if (!n_tries) {
                new_proc = proc->next;
            } else {
                /* Previous points to next */
                proc->prev->next = proc->next;

                /* If this wasn't the last element in the queue, next points to previous. */
                if (proc->next) {
                    proc->next->prev = proc->prev;
                }
            }

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

            /* Reset the anti-starvation process. */
            proc = new_proc;
            n_tries = 0;

        } else {
            /* Mem_get failed, raise n_tries and try the next process in the queue */
            n_tries++;
            proc = proc->next;
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
        printf("Give max tries to alloc memory in the new_proc queue:");
        switch (scanf("%d", &n_memory_alloc_tries)) {
            case 1:
                break;
            case EOF:
            default:
                exit(EXIT_FAILURE);
        }
        printf("Give an timeslice for the round robin:");
        switch (scanf("%d", &slice_length)) {
            case 1:
                break;
            case EOF:
            default:
                exit(EXIT_FAILURE);
        }
        set_slice(slice_length);
        first = 0;
    }

    switch (event) {
        case NewProcess_event:
            GiveMemory();
            break;
        case Time_event:
            round_robin();
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

/* A function to preform a round robbin. */
static void round_robin() {

    pcb* stub;
    pcb* old_front;

    /* Test if swapping is neccesary */
    if (ready_proc && ready_proc->next) {

        /* Move ready queue one further. */
        old_front = ready_proc;
        ready_proc = ready_proc->next;
        ready_proc->prev = NULL;

        /* Search for the end of ready queue */
        stub = ready_proc;
        while (stub->next) {
            stub = stub->next;
        }

        /* Add the pcb at the back of the ready queue. */
        stub->next = old_front;
        old_front->next = NULL;
        old_front->prev = stub;
    }

    set_slice(slice_length);
}

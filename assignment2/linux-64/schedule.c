#include <stdio.h>
#include <stdlib.h>

#include "schedule.h"
#include "mem_alloc.h"

/* This file contains a bare bones skeleton for the scheduler function
 * for the second assignment for the OSN course of the 2005 fall
 * semester.
 *
 * Author
 * G.D. van Albada
 * Section Computational Science
 * Universiteit van Amsterd
 *
 * Date:
 * October 23, 2003
 * Modified:
 * September 29, 2005
 */


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
   int n_tries = 0;
   pcb *proc = NULL;

   proc = new_proc;

   while (proc && (n_tries < n_memory_alloc_tries)) {

       /*
        * Search for a new process that should be given memory.
        * Insert search code and criteria here.
        * Attempt to allocate as follows:
        */
       index = mem_get(proc->MEM_need);
       if (index >= 0) {

           /* Allocation succeeded, now put in administration */
           proc->MEM_base = index;
           dequeue(&new_proc);

           enqueue(&ready_proc,&proc);
           proc = new_proc;
       }
       else {
           /* Increase tries and continue. */
           proc = proc->next;
           n_tries++;
       }
   }

   return;
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
        printf("Give amount of memory allocation before stopping:");
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
        /* FIXME: You may want to do this differently */
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


/* Function to enqueue a pcb from the given queue and
 * place it in the given process pointer */
static int enqueue(pcb ** proc_queue, pcb** proc) {

    pcb* stub;

    /* Check for NULL pointers and cannot put proc in empty pointer */
    if (!proc_queue || !proc || !(*proc)) {
        return EXIT_FAILURE;
    }

    /* Go to the end of the list */
    stub = *proc_queue;

    if (stub) {
        while (stub->next)
            stub = stub->next;

        /* Set the end of the queue to the process */
        stub->next = *proc;
        (*proc)->prev = stub;
    } else {
        *proc_queue = *proc;
    }

    return EXIT_SUCCESS;
}

/* Remove given item out of the queue */
static int dequeue(pcb ** proc_queue) {

    pcb *queue_element_next;
    pcb *queue_element_prev;

    /* Check for NULL pointer and cannot dequeue from empty queue. */
    if (!(proc_queue && (*proc_queue))) {
        return EXIT_FAILURE;
    }

    /* Move pointer to next value. */
    queue_element_next = (*proc_queue)->next;
    queue_element_prev = (*proc_queue)->prev;

    /* Remove all references from the removed queue item. */
    (*proc_queue)->next = NULL;
    (*proc_queue)->prev = NULL;

    /*
     * Someone where in the middle of the linked list
     * No need to move the queue pointer
     */
    if (queue_element_prev && queue_element_next) {
        queue_element_prev->next = queue_element_next;
        queue_element_next->prev = queue_element_prev;
    } else if (queue_element_prev || queue_element_next) {
        /*
         * The last element of the queue, no next value on removed element.
         * No need to move the queue pointer, there are preccesor(s).
         */
        if (queue_element_prev) {
            queue_element_prev->next = NULL;
        }
        /*
         * The first element of the queue, no prev value on the removed element.
         * Queue pointer needs to be set to the next value.
         */
        if (queue_element_next) {
            queue_element_next->prev = NULL;
            (*proc_queue) = queue_element_next;
        }
    }
    /* Only element in the queue. Set the queue pointer to NULL. */
    else {
        (*proc_queue) = NULL;
    }

    return EXIT_SUCCESS;
}

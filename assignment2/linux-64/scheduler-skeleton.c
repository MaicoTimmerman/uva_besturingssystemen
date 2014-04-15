#include <stdio.h>
#include <stdlib.h>

#include "schedule.h"
#include "mem_alloc.h"

/* This file contains a bare bones skeleton for the scheduler function
   for the second assignment for the OSN course of the 2005 fall 
   semester.
   Author:

   G.D. van Albada
   Section Computational Science
   Universiteit van Amsterdam

   Date:
   October 23, 2003
   Modified:
   September 29, 2005
*/

/* This variable will simulate the allocatable memory */

static long memory[MEM_SIZE];

/* The actual CPU scheduler is implemented here */

static void CPU_scheduler()
{
        /* Insert the code for a MLFbQ scheduler here */
}

/* The high-level memory allocation scheduler is implemented here */

static void GiveMemory()
{
   int index;
   pcb *proc1, *proc2, *proc3;

   proc2 = new_proc;
   while (proc2)
   {
       /* Search for a new process that should be given memory.
          Insert search code and criteria here.
          Attempt to allocate as follows:
        */
       index = mem_get(proc2->MEM_need);
       if (index >= 0)
       {
           /* Allocation succeeded, now put in administration
            */
           proc2->MEM_base = index;

           /* You might want to move this process to the ready
             queue now
            */
       }
   }
}

/* Here we reclaim the memory of a process after it
  has finished */

static void ReclaimMemory()
{
    pcb *proc;

    proc = defunct_proc;
    while (proc)
    {
        /* Free your own administrative structure if it exists
         */
        if (proc->your_admin)
        {
            free(proc->your_admin);
        }
        /* Free the simulated allocated memory
         */
        mem_free(proc->MEM_base);
        proc->MEM_base = -1;

        /* Call the function that cleans up the simulated process
         */
        rm_process(&proc);

        /* See if there are more processes to be removed
         */
        proc = defunct_proc;
    }
}

/* You may want to have the last word... */
 
static void my_finale()
{
    /* Your very own code goes here */
}

/* The main scheduling routine */

void schedule(event_type event)
{
    static int first = 1;

    if (first)
    {
        mem_init(memory);
        finale = my_finale;
        first = 0;
        /* Add your own initialisation code here
         */
    }

    switch (event)
    {
    /* You may want to do this differently
     */
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


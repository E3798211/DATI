
#ifndef   ECHOLOOP_H_INCLUDED
#define   ECHOLOOP_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


#ifndef   DEBUG
    #define dbg_printf(...)
#else
    #define dbg_printf(...) fprintf(stderr, __VA_ARGS__)
#endif // DEBUG

/* Most common value, simple way.
    sysconf(_SC_PAGESIZE); can be used to be sure */
#define PAGESIZE    4096

/* Assuming unique */
#define PROJ_PATH   "/"
#define PROJ_ID     777

#define N_SEMS      2
#define SEPARATOR '\n'

struct ipcs
{
    int   sem_id;
    int   service_shm_id;
    void* service_shm;
};

struct main_shm_t
{
    size_t size;
    size_t filled;
    int    id;
};

int init            (struct ipcs* info);
/* Takes shm lock inside */
int process_is_first(struct ipcs* info);
int first_action    (struct ipcs* info, char* args[], int n_args);
int late_action     (struct ipcs* info, char* args[], int n_args);

#endif // ECHOLOOP_H_INCLUDED


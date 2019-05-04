
#ifndef   ECHOLOOP_H_INCLUDED
#define   ECHOLOOP_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
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
#define PAGESIZE 4096

/* Assuming unique */
#define SERVICE_PROJ_PATH   "/"
#define PROJ_ID   777

#define N_SEMS 2

struct ipcs
{
    key_t service_key;
    key_t main_key;
    int   sem_id;
    int   service_shm_id;
    void* service_shm;
    void* main_shm;
    int   main_shm_id;
};


int init(struct ipcs* info);

#endif // ECHOLOOP_H_INCLUDED


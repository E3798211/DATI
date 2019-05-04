
#include "echoloop.h"

int init(struct ipcs* info)
{
    assert(info);

    errno = 0;
    key_t service_key = ftok(SERVICE_PROJ_PATH, PROJ_ID);
    if (service_key < 0)
    {
        perror("ftok()");
        return -EXIT_FAILURE;
    }
    info->service_key = service_key;
    dbg_printf("service_key\t= %d\n", info->service_key);

    errno = 0;
    int sem_id = semget(service_key, N_SEMS, IPC_CREAT | 0666);
    if (sem_id < 0 && errno != EEXIST)
    {
        perror("semhet()");
        if (errno == EINVAL)
            printf("Try `ipcs' to check if set already exists\n");
        return -EXIT_FAILURE;
    }
    info->sem_id = sem_id;
    dbg_printf("sem_id\t\t= %d\n", info->sem_id);

    errno = 0;
    int service_shm_id = shmget(service_key, PAGESIZE, IPC_CREAT | 0666);
    if (service_shm_id < 0)
    {
        perror("shmget()");
        return -EXIT_FAILURE;
    }
    info->service_shm_id = service_shm_id;
    dbg_printf("service_shm_id\t= %d\n", info->service_shm_id);

    errno = 0;
    void* service_shm = shmat(service_shm_id, NULL, 0);
    if (service_shm == (void*)-1)
    {
        perror("shmat()");
        return -EXIT_FAILURE;
    }
    info->service_shm = service_shm;
    dbg_printf("service_shm\t= %p\n", info->service_shm);

    return EXIT_SUCCESS;
}




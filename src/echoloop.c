
#include "echoloop.h"

int init(struct ipcs* info)
{
    assert(info);

    errno = 0;
    key_t service_key = ftok(PROJ_PATH, PROJ_ID);
    if (service_key < 0)
    {
        perror("ftok()");
        return -EXIT_FAILURE;
    }
    dbg_printf("service_key\t= %d\n", service_key);

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

int process_is_first(struct ipcs* info)
{
    assert(info);

    struct sembuf sops[] = 
    {
        { 0, 0, IPC_NOWAIT }, // if nooone exists
        { 0, 1, SEM_UNDO   }, // tell that we are the first
        { 1, 1, SEM_UNDO   }  // and lock all shared memory
    };
    
    int res;
    errno = 0;
    res = semop(info->sem_id, sops, sizeof(sops)/sizeof(sops[0]));
    if (res == 0)
        return 1;
    if (res < 0 && errno == EAGAIN)
        return 0;

    perror("semop()");
    return -EXIT_FAILURE;
}

static int reset_main_shm(struct ipcs* info)
{
    assert(info);

    int res;
    
    struct main_shm_t main_shm_info;
    memcpy(&main_shm_info, info->service_shm, sizeof(main_shm_info));

    errno = 0;
    res = shmctl(main_shm_info.id, IPC_RMID, NULL);
    if (res < 0 && errno != EIDRM && errno != EINVAL)
    {
        perror("shmctl()");
        return -EXIT_FAILURE;
    }
    
    errno = 0;
    int shm_id = shmget(IPC_PRIVATE, PAGESIZE, 0666);
    if (shm_id < 0)
    {
        perror("shmget()");
        return -EXIT_FAILURE;
    }

    main_shm_info.size   = PAGESIZE;
    main_shm_info.filled = 0;
    main_shm_info.id     = shm_id;

    memcpy(info->service_shm, &main_shm_info, sizeof(main_shm_info));

    return EXIT_SUCCESS;
}

static int resize_shm(struct ipcs* info, struct main_shm_t* main_shm_info,
                      char** old_shm)
{
    assert(info);
    assert(main_shm_info);
    assert(old_shm);

    int res;
    size_t new_size = main_shm_info->size * 2;
    int old_id = main_shm_info->id;

    errno = 0;
    int new_shm_id = shmget(IPC_PRIVATE, new_size, 0666);
    if (new_shm_id < 0)
    {
        perror("shmget()");
        return -EXIT_FAILURE;
    }

    errno = 0;
    char* new_shm = shmat(new_shm_id, NULL, 0);
    if (new_shm < 0)
    {
        perror("shmat()");
        return -EXIT_FAILURE;
    }

    memcpy(new_shm, *old_shm, main_shm_info->filled);

    main_shm_info->size = new_size;
    main_shm_info->id   = new_shm_id;

    /* New memory prepared, substituting old service info with new. */
    memcpy(info->service_shm, main_shm_info, sizeof(*main_shm_info));

    /* Cleaning old info. If process dies, garbage will left but it
        won't affect existing processes. */
    errno = 0;
    res = shmdt(*old_shm);
    if (res < 0)
    {
        perror("shmdt() failed\n");
        return -EXIT_FAILURE;
    }

    errno = 0;
    res = shmctl(old_id, IPC_RMID, NULL);
    if (res < 0)
    {
        perror("shmctl()");
        return -EXIT_FAILURE;
    }

    *old_shm = new_shm;

    return EXIT_SUCCESS;
}

static int add_args(struct ipcs* info, char* args[], int n_args)
{
    assert(info);

    struct main_shm_t main_shm_info;
    memcpy(&main_shm_info, info->service_shm, sizeof(main_shm_info));
    
    errno = 0;
    char* shm = shmat(main_shm_info.id, NULL, 0);
    if (shm == (void*)-1)
    {
        perror("shmat()");
        return -EXIT_FAILURE;
    }

    for(int i = 0; i < n_args; i++)
    {
        size_t arg_len = strlen(args[i]);
        while (arg_len + 1 > main_shm_info.size - main_shm_info.filled)
        {
            int res = resize_shm(info, &main_shm_info, &shm);
            if (res < 0)
            {
                printf("resize() failed\n");
                return -EXIT_FAILURE;
            }
        }
        memcpy(shm + main_shm_info.filled, args[i], arg_len);
        shm[main_shm_info.filled + arg_len] = SEPARATOR;
        main_shm_info.filled += arg_len + 1;
    }

    /*  Updating service info only after all operations.
        If process has died somehow before it, next singleton will
        recreate this memory. */
    memcpy(info->service_shm, &main_shm_info, sizeof(main_shm_info));

    errno = 0;
    int res = shmdt(shm);
    if (res < 0)
    {
        perror("shmdt() failed\n");
        return -EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int get_shm_lock(struct ipcs* info)
{
    assert(info);

    struct sembuf sops[] = 
    {
        { 1, 0, 0 },
        { 1, 1, SEM_UNDO }
    };

    errno = 0;
    int res = semop(info->sem_id, sops, sizeof(sops)/sizeof(sops[0]));
    if (res < 0)
    {
        perror("semop()");
        return -EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int release_shm_lock(struct ipcs* info)
{
    assert(info);
    
    struct sembuf sops[] = 
    {
        { 1, -1, IPC_NOWAIT | SEM_UNDO }
    };

    errno = 0;
    int res = semop(info->sem_id, sops, sizeof(sops)/sizeof(sops[0]));
    if (res < 0)
    {
        perror("semop()");
        return -EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int print_args(struct ipcs* info)
{
    assert(info);

    int res = 0;
    while(1)
    {
        sleep(1);

        res = get_shm_lock(info);
        if (res < 0)
        {
            printf("get_shm_lock() failed\n");
            return -EXIT_FAILURE;
        }

        struct main_shm_t main_shm_info;
        memcpy(&main_shm_info, info->service_shm, sizeof(main_shm_info));
        
        errno = 0;
        char* shm = shmat(main_shm_info.id, NULL, 0);
        if (shm == (void*)-1)
        {
            perror("shmat()");
            return -EXIT_FAILURE;
        }

        write(STDOUT_FILENO, shm, main_shm_info.filled);

        errno = 0;
        int res = shmdt(shm);
        if (res < 0)
        {
            perror("shmdt() failed\n");
            return -EXIT_FAILURE;
        }


        res = release_shm_lock(info);
        if (res < 0)
        {
            printf("release_shm_lock() failed\n");
            return -EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int first_action(struct ipcs* info, char* args[], int n_args)
{
    assert(info);
    
    int res;
    res = reset_main_shm(info);
    if (res < 0)
    {
        printf("reset_main_shm() failed\n");
        return -EXIT_FAILURE;
    }

    res = add_args(info, args, n_args);
    if (res < 0)
    {
        printf("add_args() failed\n");
        return -EXIT_FAILURE;
    }

    res = release_shm_lock(info);
    if (res < 0)
    {
        perror("release_shm_lock() failed()\n");
        return -EXIT_FAILURE;
    }

    /* Main action */
    print_args(info);

    return EXIT_SUCCESS;
}

int late_action (struct ipcs* info, char* args[], int n_args)
{
    assert(info);
    
    int res;
    res = get_shm_lock(info);
    if (res < 0)
    {
        printf("get_shm_lock() failed\n");
        return -EXIT_FAILURE;
    }

    res = add_args(info, args, n_args);
    if (res < 0)
    {
        printf("add_args() failed\n");
        return -EXIT_FAILURE;
    }

    res = release_shm_lock(info);
    if (res < 0)
    {
        printf("release_shm_lock() failed\n");
        return -EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


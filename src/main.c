
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "echoloop.h"

int main()
{
    struct ipcs info;
    int res;

    res = init(&info);
    if (res < 0)
    {
        fprintf(stderr, "init() failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


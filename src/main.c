
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "echoloop.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <args>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct ipcs info;
    int res;

    res = init(&info);
    if (res < 0)
    {
        fprintf(stderr, "init() failed\n");
        return EXIT_FAILURE;
    }

    int first = process_is_first(&info);
    if (res < 0)
    {
        fprintf(stderr, "process_is_first() failed\n");
        return EXIT_FAILURE;
    }
    dbg_printf("Process is %s\n", (first? "first":"late"));

    if (first)
        first_action(&info, &argv[1], argc - 1);
    else
        late_action (&info, &argv[1], argc - 1);

    return EXIT_SUCCESS;
}


#include <CL/cl.h>
#include <bits/getopt_core.h>
#include <stdio.h>
#include <getopt.h>

#include "driver.h"

int main(int argc, char *argv[])
{
    int opt;

    Options trj_options;
    trj_options.verbose = 0;
    trj_options.pattern = NULL;
    
    if (argc == 1)
    {
        fprintf(stdout, "trj\nMIT License (c) 2026 anstva\n");
        return 0;
    }

    while ((opt = getopt(argc, argv, "vp:")) != -1)
    {
        switch (opt) 
        {
            case 'v':
                trj_options.verbose = 1;
                break;
            case 'p':
                trj_options.pattern = optarg;
                break;
            case '?':
                return 1;
            default:
                break;
        }
    }
    
    return init_trj(trj_options);
}

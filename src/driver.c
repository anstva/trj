#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "driver.h"
#include "topology.h"
#include "search.h"

int init_trj(Options opt)
{
    if (opt.verbose == 1) 
    {
        fprintf(stdout, "Incoming pattern: %s\n", opt.pattern);
    }

    Topology target_topology = topology_split(opt.pattern, strlen(opt.pattern));

    if (opt.verbose == 1)
    {
        size_t topology_len   = target_topology.len;
        int string_padding    = 0;
        for (size_t i = 0; i < topology_len; ++i)
        {
            fprintf(stdout, "pattern data[%zu]: %*s\n", i, string_padding, target_topology.data[i]);
        }
    }

    run_search(&target_topology);

    return 0;
}

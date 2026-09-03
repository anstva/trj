#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <stddef.h>

#define GROUP_DELIMETER '"'
#define SINGLE_DELIMETER '\\'

typedef struct
{
    char** data;
    size_t len;
} Topology;

Topology topology_split(const char *argstr, const size_t argstr_len);

void topology_free(Topology *topo);

#endif

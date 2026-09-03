#ifndef DRIVER_H
#define DRIVER_H

typedef struct Options
{
    int verbose;
    char *pattern;
} Options;

int init_trj(Options opt);

#endif

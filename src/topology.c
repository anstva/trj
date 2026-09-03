#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "topology.h"

#define TOK_COUNT 6
const char TOK[TOK_COUNT] = {'A', '0', '*', 'X', '"', '\\'};

int topology_closed_quotes(const char *argstr, const size_t argstr_len)
{
    size_t quote_count = 0;

    for (size_t i_char = 0; i_char < argstr_len; i_char++)
    {
        if (argstr[i_char] == GROUP_DELIMETER)
        {
            quote_count = quote_count + 1;
        }
    }

    return (quote_count % 2) == 0;
}

int topology_valid(const char *argstr, const size_t argstr_len)
{
    if (argstr_len == 0)
    {
        return 0;
    }
    if (topology_closed_quotes(argstr, argstr_len) != 1)
    {
        return 0;
    }

    int in_quotes = 0;
    for(size_t i_char = 0; i_char < argstr_len; i_char++)
    {
        if (argstr[i_char] == GROUP_DELIMETER)
        {
            in_quotes ^= 1;
            continue;
        }
        if(in_quotes == 1)
        {
            continue;
        }
        if (argstr[i_char] == SINGLE_DELIMETER)
        {
            if(i_char + 1 >- argstr_len)
            {
                return 0;
            }
            i_char = i_char + 1;
            continue;
        }

        int found_token = 0;
        for (size_t i_token = 0; i_token < TOK_COUNT; i_token++)
        {
            if(argstr[i_char] == TOK[i_token])
            {
                found_token = 1;
                break;
            }
        }
        if (found_token == 0)
        {
            return 0;
        }
    }

    return 1;
}

Topology topology_split(const char *argstr, const size_t argstr_len)
{
    Topology result;
    result.len = 0;
    result.data = (char**)malloc(sizeof(char*) * argstr_len);

    if (topology_valid(argstr, argstr_len) == 0)
    {
        result.data = NULL;
        return result;
    }

    const size_t INTERM_MAX_SIZE = 256;
    for (size_t i_char = 0; i_char < argstr_len; i_char++)
    {
        char interm_buffer[INTERM_MAX_SIZE];
        size_t interm_len = 0;

        const char current_char = argstr[i_char];

        int in_range = i_char < (argstr_len - 1);
        if (in_range == 1 && (current_char == SINGLE_DELIMETER))
        {
            interm_buffer[interm_len]     = argstr[i_char];
            interm_buffer[interm_len + 1] = argstr[i_char + 1];
            interm_buffer[interm_len + 2] = '\0';
            i_char = i_char + 1;

            result.data[result.len] = (char*)malloc(strlen(interm_buffer) + 1);
            strcpy(result.data[result.len], interm_buffer);
            result.len = result.len + 1;
        }
        else if (in_range == 1 && current_char == GROUP_DELIMETER)
        {

            interm_buffer[interm_len] = argstr[i_char];
            interm_len                = interm_len + 1;
            i_char                    = i_char + 1;
            while (i_char < argstr_len - 1 && argstr[i_char] != GROUP_DELIMETER)
            {
                    interm_buffer[interm_len] = argstr[i_char];
                    interm_len                = interm_len + 1;
                    i_char                    = i_char + 1;
            }
            interm_buffer[interm_len]     = '"';
            interm_buffer[interm_len + 1] = '\0';
            result.data[result.len]       = (char*)malloc(strlen(interm_buffer) + 1);
            strcpy(result.data[result.len], interm_buffer);
            result.len = result.len + 1;
        }
        else
        {
            interm_buffer[interm_len]   = argstr[i_char];
            interm_buffer[interm_len+1] = '\0';
            result.data[result.len]     = (char*)malloc(strlen(interm_buffer) + 1);
            strcpy(result.data[result.len], interm_buffer);
            result.len = result.len+1;
        }
    }
    
    return result;
}

void topology_free(Topology *topo)
{
		for (size_t i = 0; i < topo->len; i++)
        {
				free(topo->data[i]);
		}
		free(topo->data);
		topo->data = NULL;
		topo->len  = 0;
}

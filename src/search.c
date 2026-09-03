#include <stddef.h>
#include <stdio.h>

#include "search.h"
#include "topology.h"

#define ALPHABETIC_COMPONENT 'A'
#define NUMERIC_COMPONENT    '0'
#define SPECIAL_COMPONENT    'X'
#define UNIVERSAL_COMPONENT  '*'

#define ALPHABETIC_SET_SIZE 52
#define NUMERIC_SET_SIZE    10
#define SPECIAL_SET_SIZE    32
#define UNIVERSAL_SET_SIZE  94

#define PREDICTION_MAX_SIZE 32

char ALPHABETIC_SET[ALPHABETIC_SET_SIZE] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
											 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
											 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
											 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
											 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
											 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
											 'w', 'x', 'y', 'z'};

char NUMERIC_SET[NUMERIC_SET_SIZE] = {'0', '1', '2', '3', '4', '5', '6',
									  '7', '8', '9'};

char SPECIAL_SET[SPECIAL_SET_SIZE] = {'!', '@', '#', '$', '%', '^', '&', '*',
									  '(', ')', '-', '_', '=', '+', '[', ']',
									  '{', '}', '|', '\\', ';', ':', '\'',
									  '\"', ',', '.', '<', '>', '/', '?', '`',
									  '~'};

char UNIVERSAL_SET[UNIVERSAL_SET_SIZE] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
										  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
										  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
										  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
										  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
										  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
										  'w', 'x', 'y', 'z', '0', '1', '2', '3',
										  '4', '5', '6', '7', '8', '9', '!', '@',
										  '#', '$', '%', '^', '&', '*', '(', ')',
										  '-', '_', '=', '+', '[', ']', '{', '}',
										  '|', '\\', ';', ':', '\'', '\"', ',',
										  '.', '<', '>', '/', '?', '`', '~'};

void build_search_space(size_t search_space[], const Topology *argtopo)
{
    char** topology_data = argtopo->data;
    size_t topology_len  = argtopo->len;

    for (size_t i = 0; i < topology_len; ++i)
    {
        switch(topology_data[i][0])
        {
            case 'A':
                search_space[i] = ALPHABETIC_SET_SIZE;
                break;
            case '0':
                search_space[i] = NUMERIC_SET_SIZE;
                break;
            case '*':
                search_space[i] = SPECIAL_SET_SIZE;
                break;
            case 'X':
                search_space[i] = UNIVERSAL_SET_SIZE;
                break;
            default:
                search_space[i] = 0;
        }
    }
}

void run_search(size_t search_space[], Topology *argtopo)
{
    size_t topology_len = argtopo->len;

    size_t working_search_space[topology_len];
    build_search_space(working_search_space, argtopo);

    size_t counter_array[topology_len];
    size_t counter_last_index = topology_len - 1;
    for (;;)
    {
        if (counter_last_index > 0 && counter_array[counter_last_index] == 0)
        {
            counter_last_index = counter_last_index - 1;
        }
        else 
        {
            break;
        }
    }

    char current_prediction[32];

}

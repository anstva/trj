#include <stddef.h>
#include <stdio.h>

#include "search.h"
#include "topology.h"
#include "gpu.h"

#define ALPHABETIC_COMPONENT   'A'
#define NUMERIC_COMPONENT      '0'
#define SPECIAL_COMPONENT      'X'
#define UNIVERSAL_COMPONENT    '*'
#define ESCAPE_QUOTE_COMPONENT '"'

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

void run_search(Topology *argtopo)
{
    size_t topology_len = argtopo->len;

    size_t working_search_space[topology_len] = {};
    build_search_space(working_search_space, argtopo);

    size_t counter_array[topology_len] = {};
    size_t counter_last_index = topology_len - 1;

    for (;;)
    {
        if (counter_last_index <= 0 || working_search_space[counter_last_index] != 0)
        {
            break;
        }

        counter_last_index = counter_last_index - 1;
    }

    init_gpu(NULL);
    uint8_t *batch_buffer = gpu_get_input_buffer();
    size_t batch_count = 0;
    const size_t batch_capacity = GPU_DEFAULT_GLOBAL_WORK_SIZE;
    size_t copy_len = (topology_len > 32) ? 32 : topology_len;

    char current_prediction[topology_len + 1];
    current_prediction[topology_len] = '\0';
    for (;;)
    {
        if (counter_array[counter_last_index] >= working_search_space[counter_last_index])
        {
            break;
        }
        for (size_t prune_index = 0; prune_index < topology_len; ++prune_index)
        {
            char current_token = argtopo->data[prune_index][0];
            switch (current_token)
            {
                case ALPHABETIC_COMPONENT:
                    current_prediction[prune_index] = ALPHABETIC_SET[counter_array[prune_index]];
                    break;
                case NUMERIC_COMPONENT:
                    current_prediction[prune_index] = NUMERIC_SET[counter_array[prune_index]];
                    break;
                case SPECIAL_COMPONENT:
                    current_prediction[prune_index] = SPECIAL_SET[counter_array[prune_index]];
                    break;
                case UNIVERSAL_COMPONENT:
                    current_prediction[prune_index] = UNIVERSAL_SET[counter_array[prune_index]];
                    break;
                case ESCAPE_QUOTE_COMPONENT:
                    char next_character = argtopo->data[prune_index][0];
                    if (next_character == ESCAPE_QUOTE_COMPONENT)
                    {
                        current_prediction[prune_index] = next_character;
                    }
                    break;
            }
        }

        /* Populate distinct candidate in work-item slot */
        if (batch_buffer)
        {
            uint8_t *slot = batch_buffer + batch_count * 32;
            memset(slot, 0, 32);
            memcpy(slot, current_prediction, copy_len);
        }
        batch_count++;

        /*
         * Launch GPU tasks in parallel when batch reaches global work size.
         * Each work item evaluates a distinct candidate.
         */
        if (batch_count >= batch_capacity)
        {
            launch_gpu_batch(batch_buffer, batch_count, topology_len);
            batch_count = 0;
        }

        counter_array[0] = counter_array[0] + 1;
        for (size_t rollover_index = 0; rollover_index < counter_last_index; ++rollover_index)
        {
            if (counter_array[rollover_index] >= working_search_space[rollover_index])
            {
                counter_array[rollover_index] = 0;
                counter_array[rollover_index + 1] = counter_array[rollover_index + 1] + 1;
            }
        }
    }

    /*
     * Launch remaining distinct candidates in final batch.
     * Each work item evaluates a distinct candidate.
     */
    if (batch_count > 0)
    {
        launch_gpu_batch(batch_buffer, batch_count, topology_len);
        batch_count = 0;
    }
}

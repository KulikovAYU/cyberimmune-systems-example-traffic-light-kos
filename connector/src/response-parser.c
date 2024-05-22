#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>  // bzero(), strlen(), strcmp()
#include <response-parser.h>
#include "json.h"

struct traffic_light_mode_t traffic_light_mode;

static void print_depth_shift(int depth)
{
    int j;
    for (j = 0; j < depth; j++)
    {
        printf(" ");
    }
}

static void process_value(json_value *value, int depth);

union {
    struct
    {
        unsigned int is_tl_mode : 1;
    };
    unsigned int flags;
} update_modes;


void update_modes_field(char *field)
{
    update_modes.flags = 0;
    if (strcmp("tl-mode", field) == 0)
    {
        update_modes.is_tl_mode = 1;
    }
}

void update_field_values_string(char *value)
{
}

void update_field_values_int(int value)
{
    if (update_modes.is_tl_mode)
    {
        traffic_light_mode.mode = value;
        fprintf(stderr, "updated mode: %d\n", traffic_light_mode.mode);
    }
}

static void process_object(json_value *value, int depth)
{
    int length, x;
    if (value == NULL)
    {
        return;
    }
    length = value->u.object.length;
    for (x = 0; x < length; x++)
    {
        D(print_depth_shift(depth);)
        D(printf("object[%d].name = %s\n", x, value->u.object.values[x].name);)
        update_modes_field(value->u.object.values[x].name);
        process_value(value->u.object.values[x].value, depth + 1);
    }
}

static void process_array(json_value *value, int depth)
{
    int length, x;
    if (value == NULL)
    {
        return;
    }
    length = value->u.array.length;
    D(printf("array\n");)
    for (x = 0; x < length; x++)
    {
        process_value(value->u.array.values[x], depth);
    }
}

static void process_value(json_value *value, int depth)
{
    if (value == NULL)
    {
        return;
    }
    if (value->type != json_object)
    {
        print_depth_shift(depth);
    }
    switch (value->type)
    {
        case json_none:
            break;
        case json_null:
            break;
        case json_object:
            process_object(value, depth + 1);
            break;
        case json_array:
            process_array(value, depth + 1);
            break;
        case json_integer:
            update_field_values_int((int)value->u.integer);
            break;
        case json_double:
            D(printf("double: %f\n", value->u.dbl);)
            break;
        case json_string:
            update_field_values_string((char *)value->u.string.ptr);
            break;
        case json_boolean:
            D(printf("bool: %d\n", value->u.boolean);)
            break;
    }
}

int parse_response(char *response)
{
    int data_size;
    json_char *json;
    json_value *value;

    // strip everything before the first opening bracket "{"
    data_size = strlen(response);
    for (int i = 0; i < data_size; i++)
    {
        if (response[i] == "{"[0])
        {
            int json_length = 0;
            for (int j = 0; j < data_size - i; j++, json_length++)
            {
                response[j] = response[i + j];
            }
            response[json_length] = 0;
            break;
        }
    }

    fprintf(stderr, "\nstripped data for parsing:\n%s\n", response);

    json = (json_char *)response;

    value = json_parse(json, data_size);

    if (value == NULL)
    {
        fprintf(stderr, "Unable to parse data\n%s\n", response);
        free(response);
        return 1;
    } else {
        process_value(value, 0);
        json_value_free(value);
    }
    return 0;
}

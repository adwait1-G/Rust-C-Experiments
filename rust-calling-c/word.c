#include <stdio.h>
#include "word.h"
#include <stdbool.h>

char *get_word (word_info_t *w_info)
{
    if (w_info == NULL)
    {
        return NULL;
    }

    return w_info->word;
}

bool get_validity (word_info_t *w_info)
{
    if (w_info == NULL)
    {
        return false;
    }

    return w_info->validity;
}

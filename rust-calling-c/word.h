#ifndef __WORD_H__
#define __WORD_H__

#include <stdbool.h>

typedef struct word_info
{
    char *word;
    bool validity;
} word_info_t;


char* get_word(word_info_t *w_info);

bool get_validity(word_info_t *w_info);


#endif /* __WORD_H__ */

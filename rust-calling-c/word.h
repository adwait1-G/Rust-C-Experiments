#ifndef __WORD_H__
#define __WORD_H__

#include <stdbool.h>

typedef struct word_info
{
    const char *word;
    bool validity;
} word_info_t;


const char* get_word(const word_info_t *w_info);

bool get_validity(const word_info_t *w_info);


#endif /* __WORD_H__ */

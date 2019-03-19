#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "markov.h"

static int to_skip(char ch)
{
    if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\"' || ch == '.' || ch == ',')
        return 1;
    return 0;
}

static int next_word(FILE *input, char *buffer, int max_len)
{
    int ch;
    int pos = 0;

    do {
        ch = fgetc(input);
        if (ch == EOF)
            return -1;
        if (!to_skip(ch))
            break;
    } while (1);

    do {
        if (pos < max_len - 1) {
            buffer[pos] = ch;
            pos ++;
        }
        ch = fgetc(input);
        if (ch == EOF)
            break;
        if (to_skip(ch))
            break;
    } while(1);
    buffer[pos] = '\0';


    if (pos > 1 && buffer[0] == '\'') {
        memmove(&buffer[1], &buffer[0], pos - 1);
        pos--;
    }

    if (pos > 1 && buffer[pos -1] == '\'') {
        buffer[pos - 1] = '\0';
        pos--;
    }

    return pos;
}

static int learn_file(struct markov_model *model, const char *file)
{
    FILE *input = fopen(file, "rb");
    do {
        char word[256];
        if (next_word(input, word, sizeof(word)) < 0)
            break;
        //printf("Got word %s\n", word);
        markov_stream_term(model, word);
    } while (1);
    fclose(input);

    markov_flush(model);

    return 0;
}

int main(int argc, char **argv)
{
    struct markov_model *model = markov_generate(NULL, 4);

#if 1 // Sherlock holmes
    learn_file(model, "texts/a_case_of_identity.txt");
    learn_file(model, "texts/bosc.txt");
    learn_file(model, "texts/five.txt");
    learn_file(model, "texts/blue.txt");
#endif
#if 1
    learn_file(model, "texts/bible.txt");
#endif

    printf("DUMP\n");
    markov_dump(model);

    srand(time(NULL));

    printf("New Story:\n");
    for (int i = 0; i < 1000; i++)
        printf("%s ", markov_guess(model));
    printf("\n");
    return 0;
}
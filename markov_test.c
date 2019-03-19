#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "markov.h"

int main(int argc, char **argv)
{
    struct markov_model *model = markov_generate(NULL, 3);
    //char *terms[] = {"foo", "blah", "wibble"};
    //char *terms2[] = {"foo", "blah", "fnord"};
    char *guess_terms[] = {"foo", "blah"};
    char *guess_terms2[] = {"foo"};

    srand(time(NULL));
    markov_stream_term(model, "foo");
    markov_stream_term(model, "blah");
    markov_stream_term(model, "wibble");
    markov_stream_term(model, "fnord");
    markov_stream_term(model, "foo");
    markov_stream_term(model, "blah");
    markov_stream_term(model, "fnord");
    markov_stream_term(model, "fnord");
    markov_stream_term(model, "fnord");
    markov_stream_term(model, "fnord");

    //markov_add_entry(model, terms);
    //markov_add_entry(model, terms2);
    //markov_add_entry(model, terms2);
    //markov_add_entry(model, terms2);
    //markov_add_entry(model, terms2);

    markov_dump(model);

    printf("Guess: %s\n", markov_guess_next(model, guess_terms, 2));
    printf("Guess2: %s\n", markov_guess_next(model, guess_terms2, 1));
    return 0;
}
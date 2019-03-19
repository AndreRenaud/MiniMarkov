#ifndef MARKOV_H
#define MARKOV_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct markov_model;

struct markov_model *markov_generate(const char *corpus, int order);
void markov_destroy(struct markov_model *);
int markov_add_term(struct markov_model *, char * const *terms);
int markov_stream_term(struct markov_model *model, const char *term);
int markov_dump(struct markov_model *model);
const char *markov_guess_next(struct markov_model *model, char * const *terms, int nterms);
const char *markov_guess(struct markov_model *model);
int markov_flush(struct markov_model *model);

#ifdef __cplusplus
}
#endif

#endif
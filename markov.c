#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "markov.h"

struct markov_term {
    int count;
    int child_total;
    struct markov_term *children;
    int nchildren;
    char *name;
};

struct markov_model {
    int order;
    struct markov_term head;

    char **stream_symbols;
    int stream_pos;

    char **output_guesses;
    int nguesses;
};

struct markov_model *markov_generate(const char *corpus, int order)
{
    struct markov_model *model;
    model = (struct markov_model *)calloc(sizeof(struct markov_model), 1);
    model->order = order;
    model->stream_symbols = (char **)calloc(sizeof(char *), order);
    model->stream_pos = 0;
    model->output_guesses = (char **)calloc(sizeof(char *), order);
    model->nguesses = 0;
    return model;
}

static void destroy_term(struct markov_term *term)
{
    for (int i = 0; i < term->nchildren; i++) {
        destroy_term(&term->children[i]);
    }
    if (term->children)
        free(term->children);
    if (term->name)
        free(term->name);
}

void markov_destroy(struct markov_model *model)
{
    destroy_term(&model->head);
    markov_flush(model);
    free(model->stream_symbols);
    free(model->output_guesses);
    free(model);
}

static struct markov_term *find_term(struct markov_term *node, const char *term, bool add)
{
    struct markov_term *ret = NULL;
    // FIXME: Should be sorting children and using bsearch
    //printf("Looking for term '%s'\n", term);
    for (int i = 0; i < node->nchildren; i++) {
        if (strcmp(node->children[i].name, term) == 0) {
            ret = &node->children[i];
            //printf("Found %s at pos %d\n", term, i);
            break;
        }
    }

    if (add && !ret) {
        //printf("Adding term '%s' to node '%s'\n", term, node->name);
        node->nchildren++;
        node->children = (struct markov_term *)realloc(node->children, node->nchildren * sizeof(struct markov_term));
        // FIXME: Handle realloc failure & re-sort
        if (!node->children)
            return NULL;
        ret = &node->children[node->nchildren - 1];
        ret->count = 0;
        ret->child_total = 0;
        ret->children = NULL;
        ret->nchildren = 0;
        ret->name = strdup(term);
    }

    return ret;
}

int markov_add_term(struct markov_model *model, char *const*terms)
{
    struct markov_term *term;
    term = find_term(&model->head, terms[0], true);
    if (!term)
        return -1;
    term->count++;
    model->head.child_total++;
    for (int i = 1; i < model->order; i++) {
        struct markov_term *next = find_term(term, terms[i], true);
        if (!next)
            return -1;
        next->count++;
        term->child_total++;
        term = next;
    }
    return 0;
}

int markov_stream_term(struct markov_model *model, const char *term)
{
    if (model->stream_pos < model->order) {
        // We haven't seen a full 'order' of symbols yet, so just tack this on to the end and move on
        model->stream_symbols[model->stream_pos] = strdup(term);
        model->stream_pos++;
    } else {
        // drop the first term from the stream, shuffle things back and add the new one
        free(model->stream_symbols[0]);
        for (int i = 1; i < model->order; i++) {
            model->stream_symbols[i - 1] = model->stream_symbols[i];
        }
        model->stream_symbols[model->order - 1] = strdup(term);
    }

    if (model->stream_pos < model->order)
        return 0;
    //printf("Adding stream:");
    //for (int i = 0; i < model->order; i++)
        //printf(" '%s'", model->stream_symbols[i]);
    //printf("\n");
    return markov_add_term(model, model->stream_symbols);
}

static int markov_dump_term(struct markov_term *term, int order, int indent)
{
    for (int i = 0; i < term->nchildren; i++) {
        printf("%*s%s [%d %d%%]\n", indent * 2, "", 
            term->children[i].name,
            term->children[i].count, term->children[i].count * 100 / term->child_total);
        if (order - 1 > 0)
            markov_dump_term(&term->children[i], order - 1, indent + 2);
    }
    return 0;
}

int markov_dump(struct markov_model *model)
{
    return markov_dump_term(&model->head, model->order, 0);
}

static struct markov_term *guess_next(struct markov_term *term)
{
    int guess, pos = 0;

    guess = rand() % term->child_total;
    //printf("Guessing %d of %d\n", guess, term->child_total);
    for (int i = 0; i < term->nchildren; i++) {
        pos += term->children[i].count;
        if (guess < pos)
            return &term->children[i];
    }
    return NULL;
}

static struct markov_term *guess_next_term(struct markov_model *model, char * const *terms, int nterms)
{
    struct markov_term *child = &model->head;
    for (int i = 0; i < nterms; i++) {
        //printf("Checking for [%d]: '%s'\n", i, terms[i]);
        child = find_term(child, terms[i], false);
        if (!child)
            return NULL;
        //printf("Walked down to %s\n", child->name);
    }

    child = guess_next(child);
    return child;
}

const char *markov_guess_next(struct markov_model *model, char * const *terms, int nterms)
{
    struct markov_term *guess = guess_next_term(model, terms, nterms);
    return guess ? guess->name : NULL;
}

const char *markov_guess(struct markov_model *model)
{
    struct markov_term *guess;
    for (int i = model->nguesses; i >= 0; i--)  {
        guess = guess_next_term(model, &model->output_guesses[model->nguesses - i], i);
        if (guess)
            break;
    }

    if (!guess)
        return NULL;

    if (model->nguesses >= model->order - 1) {
        memmove(&model->output_guesses[0], &model->output_guesses[1], (model->nguesses - 1) * sizeof(char *));
        model->nguesses--;
    }
    model->output_guesses[model->nguesses] = guess->name;
    model->nguesses++;
    
    return guess->name;
}

int markov_flush(struct markov_model *model)
{
    model->nguesses = 0;
    model->stream_pos = 0;
    for (int i = 0; i < model->order; i++) {
        if (model->stream_symbols[i]) {
            free(model->stream_symbols[i]);
            model->stream_symbols[i] = NULL;
        }
    }

    return 0;
}
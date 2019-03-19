#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "markov.h"

struct markov_term {
    int count;
    struct markov_term *children;
    int nchildren;
    char *name;
};

struct markov_model {
    int order;
    struct markov_term head;

    char **stream_symbols;
    int stream_pos;
};

struct markov_model *markov_generate(const char *corpus, int order)
{
    struct markov_model *model;
    model = (struct markov_model *)calloc(sizeof(struct markov_model), 1);
    model->order = order;
    model->stream_symbols = (char **)calloc(sizeof(char *), order);
    model->stream_pos = 0;
    return model;
}

void markov_destroy(struct markov_model *model)
{
    free(model);
}

static struct markov_term *find_term(struct markov_term *node, const char *term, bool add)
{
    struct markov_term *ret = NULL;
    // FIXME: Should be sorting children and using bsearch
    printf("Looking for term '%s'\n", term);
    for (int i = 0; i < node->nchildren; i++) {
        if (strcmp(node->children[i].name, term) == 0) {
            ret = &node->children[i];
            printf("Found %s at pos %d\n", term, i);
            break;
        }
    }

    if (add && !ret) {
        printf("Adding term '%s' to node '%s'\n", term, node->name);
        node->nchildren++;
        node->children = (struct markov_term *)realloc(node->children, node->nchildren * sizeof(struct markov_term));
        // FIXME: Handle realloc failure & re-sort
        if (!node->children)
            return NULL;
        ret = &node->children[node->nchildren - 1];
        ret->count = 0;
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
    for (int i = 1; i < model->order; i++) {
        term = find_term(term, terms[i], true);
        if (!term)
            return -1;
        term->count++;
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
    printf("Adding stream:");
    for (int i = 0; i < model->order; i++)
        printf(" '%s'", model->stream_symbols[i]);
    printf("\n");
    return markov_add_term(model, model->stream_symbols);
}

static int markov_dump_term(struct markov_term *term, int order, int indent)
{
    int total = 0;
    for (int i = 0; i < term->nchildren; i++) {
        total += term->children[i].count;
    }

    for (int i = 0; i < term->nchildren; i++) {
        printf("%*s%s [%d %d%%]\n", indent * 2, "", 
            term->children[i].name,
            term->children[i].count, term->children[i].count * 100 / total);
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
    int total = 0, guess, pos = 0;
    for (int i = 0; i < term->nchildren; i++) {
        total += term->children[i].count;
    }

    guess = rand() % total;
    printf("Guessing %d of %d\n", guess, total);
    for (int i = 0; i < term->nchildren; i++) {
        pos += term->children[i].count;
        if (guess < pos)
            return &term->children[i];
    }
    return NULL;
}


const char *markov_guess_next(struct markov_model *model, char * const *terms, int nterms)
{
    struct markov_term *child = &model->head;
    for (int i = 0; i < nterms; i++) {
        printf("Checking for %s\n", terms[i]);
        child = find_term(child, terms[i], false);
        if (!child)
            return NULL;
        printf("Walked down to %s\n", child->name);
    }

    child = guess_next(child);
    return child ? child->name : NULL;
}
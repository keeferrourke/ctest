/**
 * @file stats.c
 * @author Keefer Rourke <mail@krourke.org>
 * @brief This file contains implementation details of functions pertaining to
 *      the suite_stats_t used for reporting results from test suites.
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tdd.h"

tdd_result_t* tdd_result_new(char* name, bool ok) {
    tdd_result_t* r = malloc(sizeof(tdd_result_t));

    r->name = calloc(strlen(name) + 1, sizeof(char));
    strcpy(r->name, name);
    r->ok = ok;

    return r;
}

int tdd_result_del(tdd_result_t* result) {
    if (result == NULL) return EXIT_FAILURE;

    free(result->name);
    free(result);

    return EXIT_SUCCESS;
}

suite_stats_t* suite_get_stats(suite_t* s) {
    if (s == NULL) return NULL;

    suite_stats_t* stats = malloc(sizeof(suite_stats_t));
    stats->tests_run     = malloc(sizeof(tdd_result_t*) * (s->test_index));

    int nerr = 0, nfail = 0;
    for (int i = 0; i < s->test_index; i++) {
        runner_t* t         = s->tests[i];
        stats->tests_run[i] = tdd_result_new(t->name, s->results[i]->failed);

        test_t* r = s->results[i];
        if (r->err != 0) nerr++;
        if (r->failed != 0) nfail++;
    }
    stats->n_tests = s->n_tests;
    stats->n_ran   = s->test_index;
    stats->n_error = nerr;
    stats->n_fail  = nfail;

    return stats;
}

int suite_stats_del(suite_stats_t* stats) {
    if (stats == NULL) return EXIT_FAILURE;

    for (int i = 0; i < stats->n_ran; i++) {
        tdd_result_del(stats->tests_run[i]);
    }
    free(stats->tests_run);
    free(stats);

    return EXIT_SUCCESS;
}

char* suite_fmtstats(suite_stats_t* stats) {
#define TESTS "Ran %d of %d tests."
#define FAILS "Failed %d of %d tests. (Fatal failures: %s)"
#define ERROR "Errors during testing: %d"
#define SUCCESS "Success rate: %0.2lf"
    /* summary stats */
    char* s = calloc(strlen(TESTS FAILS ERROR) + 50, sizeof(char));
    sprintf(s, TESTS "\n" FAILS "\n" ERROR "\n" SUCCESS "\n\n", stats->n_ran,
            stats->n_tests, stats->n_fail, stats->n_tests,
            stats->fatal_failures ? "true" : "false", stats->n_error,
            stats->success_rate);

    /* at-a-glance results of each test */
    int   each_cap = 50;
    int   each_len = 0;
    char* each     = calloc(50, sizeof(char));
    for (int i = 0; i < stats->n_ran; i++) {
        char* name    = stats->tests_run[i]->name;
        int   res_len = strlen(name) + 20;
        char* res     = calloc(res_len, sizeof(char));
        if (stats->tests_run[i]->ok) {
            sprintf(res, "%s: okay\n", name);
        } else {
            sprintf(res, "%s: not okay\n", name);
        }
        if (each_cap <= each_len + res_len) {
            char* tmp =
                realloc(each, sizeof(char) * (each_len + res_len) + 50);
            if (tmp != NULL) {
                each = tmp;
            } else {
                errno = ENOMEM;
                free(s);
                return NULL;
            }
        }
        strcat(s, each);
    }

    return s;
}

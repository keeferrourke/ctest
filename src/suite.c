/**
 * @file suite.c
 * @author Keefer Rourke <mail@krourke.org>
 * @brief This file contains implementation details of functions pertaining to
 *        using suite_t structures for managing simple test suites.
 **/
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "strutil.h"
#include "tdd.h"
#include "timeutil.h"

suite_t* suite_new() {
    suite_t* s = malloc(sizeof(suite_t));
    if (s == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    s->finished   = false;
    s->n_tests    = 0;
    s->n_segv     = 0;
    s->test_index = 0;
    s->tests      = NULL;
    s->results    = NULL;
    s->outfile    = stdout;

    return s;
}

void suite_reset(suite_t* s) {
    if (s == NULL) return;

    s->finished   = false;
    s->n_segv     = 0;
    s->test_index = 0;

    for (int i = 0; i < s->n_tests; i++) {
        tdd_test_del(s->results[i]);
        s->results[i] = NULL;
    }

    return;
}

int suite_del(suite_t* s) {
    if (s == NULL) return EXIT_FAILURE;

    for (int i = 0; i < s->n_tests; i++) {
        tdd_runner_del(s->tests[i]);
        tdd_test_del(s->results[i]);
    }
    free(s->tests);
    free(s->results);
    free(s);

    return EXIT_SUCCESS;
}

void suite_done(suite_t* s) {
    if (s == NULL) return;
    s->finished = true;
    return;
}

void suite_add(suite_t* s, int n, ...) {
    if (s == NULL) return;

    /* save old index to perform update */
    int old_index = s->n_tests;

    /* update the test count and realloc test and result arrays */
    s->n_tests += n;
    runner_t** tmp_tests = realloc(s->tests, sizeof(runner_t*) * s->n_tests);
    if (!tmp_tests) {
        errno = ENOMEM;
        return;
    }
    s->tests = tmp_tests;

    test_t** tmp_results = realloc(s->results, sizeof(test_t*) * s->n_tests);
    if (!tmp_results) {
        errno = ENOMEM;
        return;
    }
    s->results = tmp_results;

    /* add tests from the va_list */
    va_list ap;
    va_start(ap, n);
    for (int i = old_index; i < s->n_tests; i++) {
        s->tests[i] = va_arg(ap, runner_t*);
    }
    va_end(ap);

    return;
}

int suite_add_test(suite_t* s, runner_t* r) {
    if (s == NULL) return EXIT_FAILURE;

    s->n_tests++;

    /* reallocate array and add new runner_t */
    runner_t** tmp_tests = realloc(s->tests, sizeof(runner_t*) * s->n_tests);
    if (tmp_tests == NULL) {
        if (s->tests != NULL) {
            free(s->tests);
        }
        errno = ENOMEM;
        return EXIT_FAILURE;
    }
    s->tests                 = tmp_tests;
    s->tests[s->n_tests - 1] = r;

    /* grow results array with the tests */
    test_t** tmp_results = realloc(s->results, sizeof(test_t*) * s->n_tests);
    if (tmp_results == NULL) {
        if (s->results != NULL) {
            free(s->results);
        }
        errno = ENOMEM;
        return EXIT_FAILURE;
    }
    s->results = tmp_results;

    return EXIT_SUCCESS;
}

int suite_run(suite_t* s, bool fatal_failures) {
    if (s == NULL) return EXIT_FAILURE;

    for (int i = 0; i < s->n_tests; i++) {
        int res = suite_next(s, fatal_failures);
        if (res != EXIT_SUCCESS) {
            return res;
        }
    }
    suite_done(s);

    return EXIT_SUCCESS;
}

/* TODO: factor this so that results may be printed to arbitrary files */
int suite_next(suite_t* s, bool fatal_failures) {
    if (s == NULL) return EXIT_FAILURE;

    /* set up test */
    runner_t* test = s->tests[s->test_index];
    test_t*   t    = tdd_test_new(test->name);

    bool bench = false;
    if (__hasprefix(test->name, "bench_")) {
        bench = true;
    }
    int crash_count = tdd_sigsegv_caught;

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags   = SA_SIGINFO;
    sa.sa_handler = &tdd_sigsegv_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        tdd_test_del(t);
        return EXIT_FAILURE;
    }

    /* run test */
    if (bench) {
        test_timer_start(t);
    }
    pthread_t thread;
    if (pthread_create(&thread, NULL, test->fn, t) != 0) {
        fprintf(stderr, "Could not create thread!\n");
        tdd_test_del(t);
        return EXIT_FAILURE;
    }
    void* retval = NULL;
    if (pthread_join(thread, &retval) != 0) {
        fprintf(stderr, "Could not join with thread!\n");
        tdd_test_del(t);
        return EXIT_FAILURE;
    }
    if (bench && t->end->tv_sec == 0 && t->end->tv_nsec == 0) {
        test_timer_end(t);
    }
    if (crash_count != tdd_sigsegv_caught) {
        t->failed = true;
        s->n_segv++;
        char* segv_msg = "encountered segmentation fault";
        if (t->fail_msg != NULL) {
            free(t->fail_msg);
        }
        t->fail_msg = calloc(strlen(segv_msg) + 1, sizeof(char));
        strncpy(t->fail_msg, segv_msg, strlen(segv_msg));
    }

    /* keep record of test results */
    s->results[s->test_index++] = t;

    /* print test results */
    FILE* f    = s->outfile;
    int   left = (s->n_tests) - (s->test_index + 1);
    char* res =
        calloc(256 + strlen(test->name) + strlen(test->desc), sizeof(char));
    int ret = EXIT_SUCCESS;
    if (t->failed) {
        sprintf(res, "fail: test %d/%d (%s): ", s->test_index, s->n_tests,
                test->name);
        __print_error(f, res);
        __print_desc(f, test->desc);
        fprintf(f, "\n");
        __INDENT(f, 6);
        __print_desc(f, t->fail_msg);
        fprintf(f, "\n");
        if (fatal_failures != 0) {
            printf("aborted with %d tests remaining.\n", left);
            ret = EXIT_FAILURE;
        }
    } else if (t->err != 0) {
        sprintf(res, "err:  test %d/%d (%s): ", s->test_index, s->n_tests,
                test->name);
        __print_warning(f, res);
        __print_desc(f, test->desc);
        fprintf(f, "\n");
        char* errstr = calloc(64, sizeof(char));
        __INDENT(f, 6);
        sprintf(errstr, "encountered %d errors.", t->err);
        __print_warning(f, errstr);
        free(errstr);
        fprintf(f, "\n");
        for (int i = 0; i < t->err; i++) {
            char* why = calloc(32 + strlen(t->err_msg[i]), sizeof(char));
            __INDENT(f, 6);
            sprintf(why, "%d. %s\n", i + 1, t->err_msg[i]);
            __print_desc(f, why);
            free(why);
        }
    } else {
        sprintf(res, "okay: test %d/%d (%s): ", s->test_index, s->n_tests,
                test->name);
        __print_success(f, res);
        __print_desc(f, test->desc);
        fprintf(f, "\n");
    }
    free(res);

    /* print benchmarking info */
    if (bench) {
        struct timespec tdiff = __timespec_minus(t->end, t->start);

        char* bench_info = calloc(64 + strlen(test->name), sizeof(char));
        __INDENT(f, 6);
        sprintf(bench_info, "bench: test (%s) took ", test->name);
        __print_desc(f, bench_info);
        char* bench_res = calloc(256, sizeof(char));
        sprintf(bench_res, "%lds %ldns\n", tdiff.tv_sec, tdiff.tv_nsec);
        __print_hilite(f, bench_res);
        free(bench_info);
        free(bench_res);
    }

    return ret;
}

libtdd
======

![repo ci status](https://img.shields.io/travis/keeferrourke/libtdd.svg)
![dev ci status](https://img.shields.io/travis/com/keeferrourke/libtdd/development.svg?label=build%20%28dev%29)

`libtdd` is a minimalist testing framework inspired by the Golang
testing pkg. It is designed to provide a framework which may be used to
build scaffolding during Test Driven Development (TDD).

This small C library attempts to provide a simple, featureful API for
creating, running, and checking the results of tests.  Simply declare
test functions, add them to a suite in your `main()`, compile and run :)


Features
--------

 * Easy API for test suite creation and execution using TDD semantics
 * Simple benchmarking
 * Pretty output with optional colour support
 * Summary statistics
 * Catch and count crashes (SIGSEGV handler)


Build and Installation
----------------------

### Meson (Ninja)

To build this library, run

    meson setup --prefix=/usr/local _build
    ninja -C _build

To install the library and documentation, run

    ninja -C _build all docs install

Builds have been tested for Fedora 29 Workstation, Ubuntu 16.04, and
macOS Mojave with both `clang` and `gcc`.

#### Build options

Build options are detailed in the `meson_options.txt` file.
You may modify them there as required.

### Make

This project was originally built with GNU Make before I migrated to
Meson/Ninja. The included Makefile will not be updated, but also won't
be removed in the foreseeable future.


Usage
-----

### Documentation

This library is extensively documented. You can generate the
documentation with `doxygen`.

Documentation is provided in the following formats:

 - HTML
 - PDF (requires `LaTeX`)
 - man pages (run `man -m _build/docs/man $page` to view)

To build the documentation, run the following:

    ninja -C _build docs

It will be output to `_build/docs`.

### Example program

A simple example program that demonstrates all features provided by this
library can be found in the `examples/` directory of this project.

It is built as part of the default build target,

### Conventions to follow

#### Test functions

Every testing function is defined as a `void* test_myfunc(void* t)`
where the parameter `t` is a `test_t` that records information about
the test, including failure and error states, and start and end time
of each test. The parameter and return types are `void*` because tests
are run in `pthreads`. Despite this, there is no need to cast `t` when
calling the provided API functions.

A test is assumed to have succeeded if it runs to completion without
raising any failure or error flags. A test can be marked as a failure
by a call to `test_fail(t, "reason")`; you should return from the test
function shortly after this call is made. Non-critical errors that
should not end a test can be recorded by `test_error(t, "reason")`.

The macro `test_fatal(t, "reason")` exists for convenience so you don't
need to return from a failure condition (this is handled automatically)
but should be used sparingly as you won't have an opportunity to run
clean-up code if it executes.

The start of a test can be optionally flagged by `test_timer_start(t)`
and the end the test can be similarly flagged by `test_timer_end(t)`,
if the name of your test is prefixed by `bench_` then these functions
will be called for you automatically.

#### Benchmarked functions

If benchmarking a test, then the `test_timer_start(t)` and
`test_timer_end(t)` function calls will automatically be called if
the test name is prefixed by `bench_`. These times will be automatically
recorded by the suite and reported at the end.

Calls to `test_timer_start(t)` and `test_timer_end(t)` may still be made
to override the start and end times if the benchmarks require setup and
teardown code that should not be included in the recorded time.

i.e. a benchmarking function may be simply defined as

    void* bench_func(void* t) {
        ...
        return NULL;
    }

and can be added to a test suite as a benchmarking function by

    suite_addtest(s, newtest("bench_one", &bench_func));

After execution of each benchmark function, a short summary of runtime
is printed.


Future development
------------------

There are no bugs to speak of at the moment :)

### Possible feature work

 * It might be nice to optionally run tests in parallel; currently all
   tests run sequentially in the order they are added to the suite
 * I'll probably factor out the reporter interface so that new reporters
   can be written and used to display results
 * It might be good to specify how long a benchmarked function has to
   run before it might be considered a failure due to poor performance


License information
-------------------

Copyright (c) 2018-2019 Keefer Rourke <mail@krourke.org>

This software is released under the ISC License. See [LICENSE](LICENSE)
for more details.


Alternatives
------------

This project aims to realize a modern, easy to use solution for C
testing frameworks. If you don't want to use this, there are some
alternatives that I like:

 - [`rcunit`](https://github.com/jecklgamis/rcunit)
    + Seems very similar to the goals of my project;
      I wish I'd known about it before :wink:
 - [`minunit`](http://www.jera.com/techinfo/jtns/jtn002.html)
    + For the ultra minimal
 - [`check`](https://libcheck.github.io/check/)
    + Includes an extensive assertion library

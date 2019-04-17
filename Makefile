# libtdd Makefile
# Keefer Rourke <mail@krourke.org>
#
# This Makefile creates dynamic and static library binaries for `libctest`.
# Libraries are output to $PWD/lib
#
# See LICENSE file included at the project root.

# specify the shell, in case the SHELL variable is not set or is not
# inherited from the environment
SHELL = /bin/bash

# determine platform
UNAME = $(shell uname -s)

# let make determine the compiler and archiver;
# if it complains, try uncommenting the following lines
# CC = gcc
# AR = ar

# set suffix list, to prevent confusion between different make programs
# line 15 clears an implied suffix list, and line 16 sets a new one
.SUFFIXES:
.SUFFIXES: .c .h .o

# this library conforms to the C99 standard with POSIX_C_SOURCE
# POSIX_C_SOURCE is defined in order to use the safer sigaction interface for
# signal handling, to expose pthread_kill, and to enable clock_gettime() which
# is used to populate timespec structs
STD = -std=c99 -D_POSIX_C_SOURCE=199506L

# library project structure
WD	:= $(PWD)
INCLDIR	= $(WD)/include
SRCDIR	= $(WD)/src
BUILDDIR = $(WD)/_build
OBJDIR	= $(BUILDDIR)/obj
OUTDIR	= $(BUILDDIR)/lib

# DOCDIR and TEXDIR must match the appropriate directories specified in the
# Doxyfile; TEXDIR is a subdirectory of DOCDIR
DOCDIR = docs
BUILTDOCDIR = $(BUILDDIR)/docs

# library output
OUTNAME	= libtdd
LIB	= $(addprefix $(OUTDIR)/, $(OUTNAME))

# files; here all object files will be stored in OBJDIR, with the same names
# as corresponding c files from SRCDIR
SRC	= $(wildcard $(SRCDIR)/*.c)
_OBJS	= $(patsubst $(SRCDIR)/%.c, %.o, $(SRC))
OBJS	= $(addprefix $(OBJDIR)/, $(_OBJS))

# compilation flags
# add extra flags on the command line by DEFINE=...
CFLAGS	= -Wall -Wextra -pedantic -g $(STD) $(DEFINE)
OFLAGS	= -pthread
INCLUDE = -I$(INCLDIR)

.PHONY: all lib dylib

# this target compiles the final library binaries and generates documentation
all: lib dylib doc

lib: $(OBJS) $(OUTDIR)
	@$(AR) crs $(LIB).a $(OBJS)
	@echo "Compiled $(OUTNAME).a to $(OUTDIR)"

dylib: $(OBJS) $(OUTDIR)
ifeq ($(UNAME),Darwin)
	@$(CC) -shared -o $(LIB).dylib $(OBJS)
	@echo "Compiled $(OUTNAME).dylib to $(OUTDIR)"
else
	@$(CC) -shared -o $(LIB).so $(OBJS)
	@echo "Compiled $(OUTNAME).so to $(OUTDIR)"
endif

# automatically compile all objects
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDE) -fpic -c $< -o $@

# the following targets simply ensure that the expected directories exist
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

$(BUILTDOCDIR):
	@mkdir -p $(BUILTDOCDIR)

# generate docs with doxygen
# this is intended to be used with a Doxyfile that specified LaTeX output
# modify as required for different documentation formats
#
# inelegant, but if doxygen fails for some reason, it is not the end of the
# world
.PHONY: doc doc-clean
doc: $(BUILTDOCDIR) $(DOCDIR)/Doxyfile.in
	-@cp $(DOCDIR)/Doxyfile.in Doxyfile
	-@sed -i "s/@HAS_DOT@/NO/g" Doxyfile
	-@sed -i "s/@HTML_PAGES@/YES/g" Doxyfile
	-@sed -i "s/@GEN_LATEX@/NO/g" Doxyfile
	-@sed -i "s/@MAN_PAGES@/YES/g" Doxyfile
	-@sed -i "s/@PROJECT_NAME@/$(OUTNAME)/g" Doxyfile
	-@sed -i "s/@VERSION@//g" Doxyfile
	-@sed -i "s|@DOCS_OUTPUT_DIR@|$(BUILTDOCDIR)|g" Doxyfile
	-@sed -i "s|@INCLUDE_DIR@|$(INCLDIR)|g" Doxyfile
	-@sed -i "s|@README_PATH|README.md|g" Doxyfile
	 @doxygen 2>/dev/null 1>&2
	-@rm Doxyfile
	-@echo 'Generated application internal documentation.'

doc-clean:
	-@rm -r $(BUILTDDOCDIR)

# clean entire project directory
.PHONY: clean cleaner
clean:
	-@rm -rf $(OBJDIR)

cleaner:
	-@rm -rf $(OUTDIR)

# check code quality
.PHONY: cppcheck memcheck
cppcheck:
	cppcheck --enable=all --language=c --std=c99 \
		--suppress=missingInclude --suppress=unusedFunction \
		$(SRC) -i $(INCLDIR)

# print target; useful for debugging this Makefile
print-%:
	@echo $($*)


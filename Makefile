NAME = floodus
SRCDIR = src/
BIN = bin/
DOCS = doc/
INCL = include/
LIB = lib/
RES = res/
COVERAGE = coverage.html/

FILES := $(shell find $(SRCDIR) -name '*.c')
OBJ:= $(FILES:$(SRCDIR)%.c=$(BIN)%.o)

CC = gcc
FLAGS = -Wall -Wextra -Werror -fprofile-arcs -ftest-coverage
LDLIBS = -pthread -D_REENTRANT -lm -lncurses

.PHONY: all
all:
	@printf "%s\n" $(OBJ)

$(NAME): $(OBJ)
	@printf "[\e[1;34mEn cours\e[0m] Assemblement\n"
	$(CC) -o $(NAME) $(FLAGS) -I $(INCL) $(OBJ) $(LDLIBS)
	@printf "[\e[1;32mOK\e[0m] Assemblement finie\n"

$(BIN)%.o: $(SRCDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(FLAGS) $(LDLIBS) -I $(INCL) -o $@ $<
 
.PHONY: clean
clean:
	@printf "[\e[1;34mEn cours\e[0m] Suppression des binaires\n"
	@rm -rf $(BIN)
	@rm -rf $(COVERAGE)
	@rm -rf coverage.info
	@printf "[\e[1;32mOK\e[0m] Suppression finie\n"

.PHONY: cleandoc
cleandoc:
	@printf "[\e[1;34mEn cours\e[0m] Suppression de la documentation\n"
	@rm -rf $(DOCS)
	@printf "[\e[1;32mOK\e[0m] Suppression finie\n"

.PHONY: cleanall
cleanall: clean cleandoc
	@rm -rf $(NAME)

.PHONY: re
re: cleanall $(NAME)

.PHONY: doc
doc: cleandoc
	@printf "[\e[1;34mEn cours\e[0m] Création de la documentation\n"
	@doxygen documentation
	@printf "[\e[1;32mOK\e[0m] Création finie\n"

.PHONY: zip
zip:
	@printf "[\e[1;34mEn cours\e[0m] Début du zippage\n"
	@zip -r $(NAME).zip README Makefile $(SRCDIR) documentation $(LIB) $(RES) $(INCL)
	@printf "[\e[1;32mOK\e[0m] Zippage finie\n"

.PHONY: coverage
coverage:
	@printf "[\e[1;34mEn cours\e[0m] Création du rapport de code coverage\n"
	@rm -rf coverage coverage.info
	@lcov -c --directory bin --output-file coverage.info
	@genhtml coverage.info --output-directory $(COVERAGE)
	@printf "[\e[1;32mOK\e[0m] Rapport dans coverage\n"


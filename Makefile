NAME = floodus
SRCDIR = src/
BIN = bin/
DOCS = doc/
INCL = include/
LIB = lib/
RES = res/

FILES := $(shell find $(SRCDIR) -name '*.c')
OBJ:= $(FILES:$(SRCDIR)%.c=$(BIN)%.o)

CC = gcc
FLAGS = -Wall -Wextra -Werror
LDLIBS = 

.PHONY: all
all:
	@printf "%s\n" $(OBJ)

$(NAME): $(OBJ)
	@echo "[\e[1;34mEn cours\e[0m] Assemblement"
	$(CC) -o $(NAME) $(FLAGS) -I $(INCL) $(LDLIBS) $(OBJ)
	@$(CC) -o $(NAME) $(FLAGS) -I $(INCL) $(LDLIBS) $(OBJ)
	@echo "[\e[1;32mOK\e[0m] Assemblement finie"

$(BIN)%.o: $(SRCDIR)%.c
	mkdir -p $(dir $@)
	$(CC) -c $(FLAGS) $(LDLIBS) -I $(INCL) -o $@ $<
 
.PHONY: clean
clean:
	@echo "[\e[1;34mEn cours\e[0m] Suppression des binaires"
	@rm -rf $(BIN)
	@echo "[\e[1;32mOK\e[0m] Suppression finie"

.PHONY: cleandoc
cleandoc:
	@echo "[\e[1;34mEn cours\e[0m] Suppression de la documentation"
	@rm -rf $(DOCS)
	@echo "[\e[1;32mOK\e[0m] Suppression finie"

.PHONY: cleanall
cleanall: clean cleandoc
	@rm -rf $(NAME)

.PHONY: re
re: cleanall $(NAME)

.PHONY: doc
doc: cleandoc
	@echo "[\e[1;34mEn cours\e[0m] Création de la documentation"
	@doxygen documentation
	@echo "[\e[1;32mOK\e[0m] Création finie"

.PHONY: zip
zip:
	@echo "[\e[1;34mEn cours\e[0m] Début du zippage"
	@zip -r $(NAME).zip README Makefile $(SRCDIR) documentation $(LIB) $(RES) $(INCL)
	@echo "[\e[1;32mOK\e[0m] Zippage finie"

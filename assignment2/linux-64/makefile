# Enter your UVAID here
UVAID=10542590
NAME=Timmerman&Kruger
EXERCISE=2
PROGNAME=schedsim2

MKDIR=mkdir
PACK=tar -zcvf
CC=gcc
PREFIXCFLAGS=-Wall -Wextra -Werror -ggdb -pedantic-errors -std=c99 -O2
LIBS=-lm
CP=cp -r
RM=rm -rf
OFILES=schedule.o mem_alloc.o simul2014.o mt19937ar.o

$(PROGNAME): $(OFILES)
	$(CC) $(PREFIXCFLAGS) $(OFILES) -o $(PROGNAME) $(LIBS)

schedule.o: schedule.c
	$(CC) $(PREFIXCFLAGS) -c $< -o $@

.PHONY: run
run: $(PROGNAME)
	./$(PROGNAME)

.PHONY: debug
debug: $(PROGNAME)
	./gdb $(PROGNAME)

.PHONY: clean
clean:
	$(RM) schedule.o $(UVAID)_$(PROGNAME).tar.gz

.PHONY: remove
remove:
	$(RM) schedule.o $(PROGNAME) $(UVAID)_$(PROGNAME).tar.gz

.PHONY: tarball
tarball: $(PROGNAME)
	$(MAKE) remove
	$(MKDIR) OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME)
	$(CP) * OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME) || true
	$(PACK) OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME).tar.gz OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME)/*
	$(RM) OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME)

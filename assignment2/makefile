# Enter your UVAID here
UVAID=10542590
NAME=Timmerman&Kruger
EXERCISE=2
PROGNAME=scheduling

MKDIR=mkdir
PACK=tar -zcvf
CC=gcc
PREFIXCFLAGS=-Wall -Wextra -Werror -ggdb -pedantic-errors -std=c99
LIBS=
CP=cp -r
RM=rm -rf
OFILES=$(subst .c,.o,$(wildcard *.c))

$(PROGNAME): $(OFILES)
	$(CC) $(PREFIXCFLAGS) $(OFILES) -o $(PROGNAME) $(LIBS)

%.o: %.c
	$(CC) $(PREFIXCFLAGS) -c $< -o ./$@

.PHONY: run
run: $(PROGNAME)
	./$(PROGNAME)

.PHONY: clean
clean:
	$(RM) *.o $(UVAID)_$(PROGNAME).tar.gz

.PHONY: remove
remove:
	$(RM) *.o $(PROGNAME) $(UVAID)_$(PROGNAME).tar.gz

.PHONY: tarball
tarball: $(PROGNAME)
	$(MAKE) remove
	$(MKDIR) OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME)
	$(CP) * OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME) || true
	$(PACK) OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME).tar.gz OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME)/*
	$(RM) OS_Opgave$(EXERCISE)_$(UVAID)_$(NAME)

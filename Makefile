MODULE=constwidth
SRCS=main.c

OBJS=$(SRCS:.c=.o)

CFLAGS+=-fPIC -std=c99 -Wall -Wextra
CFLAGS+=$(shell pkgconf --cflags cairo)
CFLAGS+=$(shell pkgconf --cflags pangocairo)

LDFLAGS+=-shared -Wl,-Bsymbolic
LDLIBS+=$(shell pkgconf --libs cairo)
LDLIBS+=$(shell pkgconf --libs pangocairo)

PREFIX?=/usr/local
MODULEDIR?=$(PREFIX)/lib/blockbar/modules

.PHONY: all install-src uninstall-src install-so uninstall-so install uninstall clean

ifeq ($(DEBUG),1)
CFLAGS+=-Og -ggdb
endif

all: $(MODULE).so

$(MODULE).so: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

install-src:
	mkdir -p "$(MODULEDIR)/src/$(MODULE)"
	cp -fp Makefile "$(MODULEDIR)/src/$(MODULE)"
	cp -fp $(SRCS) "$(MODULEDIR)/src/$(MODULE)"

uninstall-src:
	rm -rf "$(MODULEDIR)/src/$(MODULE)"

install-so: $(MODULE).so
	mkdir -p "$(MODULEDIR)"
	cp -fp "$(MODULE).so" "$(MODULEDIR)"

uninstall-so:
	rm -f "$(MODULEDIR)/$(MODULE).so"

install: install-src install-so

uninstall: uninstall-src uninstall-so

clean:
	rm -f $(OBJS) $(MODULE).so

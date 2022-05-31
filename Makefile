include config.mk

SRC = ahc.c parsemsg.c
OBJ = ${SRC:.c=.o}

all: options libahc.so

options:
	@echo ahc build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "CC      = ${CC}"
	@echo "LDFLAGS = ${LDFLAGS}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk Makefile

ahc.o: parsemsg.h sdk.h util.h
parsemsg.o: parsemsg.h

libahc.so: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f libahc.so ${OBJ} ahc.tar.gz

dist: clean
	mkdir -p ahc
	cp -R Makefile ahc.c config.mk load.sh \
	      parsemsg.c parsemsg.h sdk.h \
	      unload.sh util.h ahc
	tar -cf ahc.tar ahc
	gzip ahc.tar
	rm -rf ahc

.PHONY: all options clean dist

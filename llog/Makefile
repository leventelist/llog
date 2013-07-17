PREFIX=/usr/local
CC=gcc
CFLAGS=-O2 -g -Wall
LDFLAGS=-L/usr/lib -lm -lreadline

BINDIR=${PREFIX}/bin

SOURCES=main.c csv.c conf.c

TEST_SOURCES=

EXECUTABLE=llog
TEST_EXECUTABLE=test_out


TARGET=$(patsubst %.c,%.o,${SOURCES})
TEST_TARGET=$(patsubst %.c,%.o,${TEST_SOURCES})

default: ${EXECUTABLE}

${EXECUTABLE}: ${TARGET}
	${CC} ${CFLAGS} ${LDFLAGS} -o ${EXECUTABLE} ${TARGET}

test:	${TEST_TARGET}
	${CC} ${CFLAGS} ${LDFLAGS} -o ${TEST_EXECUTABLE} ${TEST_TARGET}
	./${TEST_EXECUTABLE}

%.o:    %.c
	${CC} ${CFLAGS} -o $*.o -c $*.c

clean:	test_clean
	rm -f ${TARGET} ${EXECUTABLE}

test_clean:
	rm -f ${TEST_TARGET} ${TEST_EXECUTABLE}

install: ${EXECUTABLE}
	install -m 755 ${EXECUTABLE} ${BINDIR}

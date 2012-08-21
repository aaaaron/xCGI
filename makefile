LIBNAME = libxcgi.a
LIBS = -L. -lxcgi
CFLAGS =-g
CC=gcc
AR=ar rc

.c.o: ${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -c $<

all:${LIBNAME} 

${LIBNAME}: xcgi.o
	rm -f ${LIBNAME}
	${AR} ${LIBNAME} xcgi.o

test: test.o xcgi.o
	${CC} ${CFLAGS} -o test test.o ${LIBS}

clean:
	-rm -f *.o 

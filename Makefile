all: ssr-one-util

#prs.o: prs.c prs.h
#	cc -c prs.c -o prs.o
#one.o: one.c one.h
#	cc -c one.c -o one.o

ssr-one-util: ssr-one-util.c prs.c prs.h one.c one.h
	gcc -O2 -Wall ssr-one-util.c prs.c one.c -o ssr-one-util

clean:
	rm -f *.o ssr-one-util

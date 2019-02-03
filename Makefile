CC = cc
CFLAGS = -std=c99 -pedantic -Wall -Werror -D_POSIX_C_SOURCE=200809L -DUNIT_TEST

SRCS = \
 http.c \
 log.c \
 main.c \
 url.c

OBJS = $(SRCS:.c=.o)

MAIN = http_client

.PHONY: depend clean

all: $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $^

# DO NOT DELETE THIS LINE -- make depend needs it

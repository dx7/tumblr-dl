CFLAGS=-Wall -g -I/usr/local/include
LDFLAGS=-L/usr/local/lib
LDLIBS=-lcurl

all: tumblr-dl

clean:
	rm -f tumblr-dl

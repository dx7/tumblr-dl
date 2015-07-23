LIBS=-lcurl $(OPTLIBS)

bin/tumblr-dl:
	@mkdir -p bin
	$(CC) src/tumblr-dl.c -o bin/tumblr-dl $(LIBS)

clean:
	rm -rf bin

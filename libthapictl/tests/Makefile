
.PHONY: all
all: hello hello-ze

hello-tp.o: hello-tp.c hello-tp.h
	gcc -c -I. hello-tp.c

hello.o: hello.c
	gcc -c -I../ hello.c

hello: hello.o hello-tp.o
	gcc -o hello hello.o hello-tp.o -llttng-ust -ldl -L../ -lthapi-ctl -llttng-ctl

hello-ze: hello-ze.cxx
	icpx -fsycl -o $@ $< -I../ -llttng-ctl -Wl,-rpath,$(THAPI_HOME)/lib -L$(THAPI_HOME)/lib -lthapi-ctl

.PHONY: clean
clean:
	rm -rf hello *.o

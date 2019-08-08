all: tracer.so

tracer.c: gen.rb
	ruby gen.rb > tracer.c

tracer.so: tracer.c
	gcc -o tracer.so -shared -fPIC -Wl,--version-script,tracer.map tracer.c -ldl

clean:
	rm -f tracer.c tracer.so

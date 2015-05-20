all: clean poe

run:
	./build/poe

clean:
	rm -rf build/*

memcheck: example
	valgrind --tool=memcheck --leak-check=full build/poe

example: clean poe.o transport.o
	g++ build/poe.o build/transport.o examples/simple.cc -I../libuv/include -Wl,--start-group ../libuv/out/Debug/libuv.a -Wl,--end-group -lpthread -std=c++11 -o ./build/poe
	
poe.o:
	g++ -std=c++11 -c poe.cc -o ./build/$@

transport.o:
	g++ -std=c++11 -c transport.cc -o ./build/$@
	
%: %.cc
	g++ -std=c++11 $< -o $@
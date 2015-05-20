all: clean poe

clean:
	rm -rf poe build/*

memcheck: test
	valgrind --tool=memcheck --leak-check=full ./test

test: clean poe.o transport.o
	g++ build/poe.o build/transport.o test.cc -I../libuv/include -Wl,--start-group ../libuv/out/Debug/libuv.a -Wl,--end-group -lpthread -std=c++11 -o ./test
	
poe.o:
	g++ -std=c++11 -c poe.cc -o ./build/$@

transport.o:
	g++ -std=c++11 -c transport.cc -o ./build/$@
	
%: %.cc
	g++ -std=c++11 $< -o $@
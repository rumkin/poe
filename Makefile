all: clean poe

run:
	./build/poe

clean:
	rm -rf build/*

memcheck: example
	valgrind --tool=memcheck --leak-check=full build/poe

example: clean build
	g++ build/poe.o build/transport.o examples/simple.cc -Ideps/libuv/include -Wl,--start-group deps/libuv/out/Debug/libuv.a -Wl,--end-group -lpthread -std=c++11 -o ./build/poe
	
poe.o:
	g++ -std=c++11 -c poe.cc -o ./build/$@

transport.o:
	g++ -std=c++11 -c transport.cc -o ./build/$@

# Manage dependencies	
deps: deps_dir deps-libuv
	
deps_dir:
	rm -rf deps && mkdir deps

deps-libuv: deps-libuv-repo deps-libuv-gyp

deps-libuv-repo:
	git clone https://github.com/libuv/libuv.git deps/libuv

deps-libuv-gyp:
	git clone https://chromium.googlesource.com/external/gyp.git deps/libuv/build/gyp
	
# Build

build: build-deps build-objects

build-deps: build-libuv
	
build-libuv:
	cd deps/libuv && ./gyp_uv.py -f make && make -C out
	
build-objects: poe.o transport.o

%: %.cc
	g++ -std=c++11 $< -o $@
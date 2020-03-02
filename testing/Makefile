.PHONY: all clean-compile compile gen mk clean

cmake_args = -DCMAKE_BUILD_TYPE=Debug

all: compile

#test: compile
#	cd build && make

clean-compile: clean compile

compile: gen
	cd build && cmake --build .

gen: mk
	cd build && cmake $(cmake_args) ..

mk:
	mkdir -p build

clean:
	rm -rf build

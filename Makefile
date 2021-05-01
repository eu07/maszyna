.PHONY=clean

clean:
	rm -rf build

build:
	mkdir -p build
	cd build && cmake ../

eu07: build
	cd build && make

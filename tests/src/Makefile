.PHONY: all build clean run unit integration

all: run

build:
	mkdir -p build \
		&& cd build \
		&& cmake .. \
		&& make

clean:
	rm -rf build

run: unit
ifeq ($(shell uname -s),Darwin)
run: integration
endif

unit: build
	./build/test
	./build/access_test

integration: build
	bash scripts/volume_integration_test.sh $(INTEGRATION_ARGS)

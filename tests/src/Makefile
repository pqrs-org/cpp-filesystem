.PHONY: all build clean run unit integration osx-unit osx-integration

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
run: osx-unit osx-integration
endif

unit: build
	./build/test
	./build/access_test

osx-unit: build
	./build/osx_volume_test
	/usr/bin/python3 scripts/osx_volume_cleanup_test.py

osx-integration: build
	bash scripts/osx_volume_integration_test.sh $(INTEGRATION_ARGS)

integration: osx-integration

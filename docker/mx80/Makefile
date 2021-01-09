current_dir:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

.PHONY: all

target: all

deps:
	wget -c https://www.buildroot.org/downloads/buildroot-2020.02.8.tar.bz2
	if [ ! -d src/buildroot-2020.02.8 ]; then tar -C src/ -jxf buildroot-2020.02.8.tar.bz2; fi
	git clone https://github.com/halmartin/meraki-builder.git src/meraki-builder

docker-image:
	docker build -t ubuntu1804-buildroot -f Dockerfile .

docker-build:
	docker run -it --rm -v $(current_dir)/src:/src --entrypoint=/src/meraki-builder/buildroot/board/meraki/mx80/buildroot.sh ubuntu1804-buildroot

all: docker-image docker-build

current_dir:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BUILDROOT_VERSION=2021.02.1

.PHONY: all

target: all

deps:
	wget -c https://www.buildroot.org/downloads/buildroot-$(BUILDROOT_VERSION).tar.bz2
	if [ ! -d src/buildroot ]; then mkdir -p src/buildroot && tar -C src/buildroot --strip-components=1 -jxf buildroot-$(BUILDROOT_VERSION).tar.bz2; fi
	if [ ! -d src/meraki-builder ]; then git clone https://github.com/halmartin/meraki-builder.git src/meraki-builder; else cd src/meraki-builder && git pull && cd ../../; fi
	cd src/buildroot/board && ln -sf ../../meraki-builder/buildroot/board/meraki . && cd ../../../
	cd src/buildroot && for patch in $(ls -d ../meraki-builder/buildroot/patches/*.patch); do patch -p1 < $patch; done && cd ../../

docker-image:
	docker build -t ubuntu1804-buildroot -f Dockerfile ${mktemp -d}

docker-build-dbg:
	docker run -it --rm -v $(current_dir)/src:/src --entrypoint=/bin/bash ubuntu1804-buildroot

all: docker-image docker-build

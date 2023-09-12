current_dir:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
patches:=$(shell find src/meraki-builder/buildroot/patches/ -type f -name "*.patch" -exec basename {} \;)
BUILDROOT_VERSION=2023.02.4

.PHONY: all

target: all

deps:
	wget -c https://www.buildroot.org/downloads/buildroot-$(BUILDROOT_VERSION).tar.xz
	if [ ! -d src/buildroot ]; then mkdir -p src/buildroot && tar -C src/buildroot --strip-components=1 -Jxf buildroot-$(BUILDROOT_VERSION).tar.xz; fi
	if [ ! -d src/meraki-builder ]; then git clone https://github.com/halmartin/meraki-builder.git src/meraki-builder; else cd src/meraki-builder && git pull && cd ../../; fi
	cd src/buildroot/board && ln -sf ../../meraki-builder/buildroot/board/meraki . && cd ../../../
	cd src/buildroot/package && ln -sf ../../meraki-builder/buildroot/packages/pd690xx . && cd ../../../
	cd src/buildroot && for p in $(patches); do patch -p0 < ../meraki-builder/buildroot/patches/$$p; done && cd ../../

docker-image:
	docker build -t ubuntu1804-buildroot -f Dockerfile ${mktemp -d}

docker-build-dbg:
	docker run -it --rm -v $(current_dir)/src:/src --entrypoint=/bin/bash ubuntu1804-buildroot

all: docker-image docker-build

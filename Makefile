BOARDS   := $(shell ls buildroot/board/meraki)
PACKAGES := $(shell ls buildroot/packages)
PATCHES  := $(shell ls buildroot/patches/*.patch)

IMAGE   := "ubuntu1804-buildroot"
WORKDIR := "/src/buildroot"

.PHONY: catch all docker-image docker-build $(BOARDS)

catch:
	$(error "please specify a board name from one of: $(BOARDS)")

all: docker-image docker-build

docker-image:
	docker build -t $(IMAGE) -f Dockerfile .

docker-build:
	docker run -it --rm \
	-e KCONFIG_OVERWRITECONFIG=0 \
	-v $$PWD/buildroot/board/meraki:$(WORKDIR)/board/meraki \
	-v $$PWD/buildroot/board/meraki/$(BOARD)/buildroot-config:$(WORKDIR)/.config \
	-v $$PWD/buildroot/patches:$(WORKDIR)/patches \
	-v $$PWD/output:$(WORKDIR)/output \
	$(shell for p in $(PACKAGES); do echo -v $$PWD/buildroot/packages/$$p:$(WORKDIR)/package/$$p; done) \
	$(IMAGE) bash -c 'for p in $(PATCHES); do patch -p0 < $${p#*/}; done && bash'

$(BOARDS):
	$(MAKE) BOARD=$@ all

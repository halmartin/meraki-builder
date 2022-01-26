################################################################################
#
# click
#
################################################################################

CLICK_SITE = https://github.com/kohler/click.git
CLICK_SITE_METHOD = git
CLICK_VERSION = 593d10826cf5f945a78307d095ffb0897de515de
CLICK_INSTALL_STAGING = YES
CLICK_DEPENDENCIES = \
	host-automake libpcap
CLICK_LICENSE = MIT
CLICK_LICENSE_FILES = LICENSE
CLICK_CONF_OPTS = --disable-linuxmodule \
	--disable-test \
	--enable-ip6 \
	--enable-json \
	--enable-tools=no \

# The click build system cannot build both the shared and static
# libraries. So when the Buildroot configuration requests to build
# both the shared and static variants, we build only the shared one.
ifeq ($(BR2_SHARED_LIBS)$(BR2_SHARED_STATIC_LIBS),y)
CLICK_COMPONENT_TYPE = lib-shared
else
CLICK_COMPONENT_TYPE = lib-static
endif

define CLICK_CONFIGURE_CMDS
        (cd $(@D); $(TARGET_MAKE_ENV) ./configure \
                --prefix=/usr \
                $(CLICK_CONF_OPTS) \
        )
endef

# Use $(MAKE1) since parallel build fails
define CLICK_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) PREFIX=/usr \
		COMPONENT_TYPE=$(CLICK_COMPONENT_TYPE)
endef

define CLICK_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) \
		$(MAKE) -C $(@D) PREFIX=/usr DESTDIR=$(STAGING_DIR) \
		COMPONENT_TYPE=$(CLICK_COMPONENT_TYPE) install
endef

define CLICK_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV
		$(MAKE) -C $(@D) PREFIX=/usr DESTDIR=$(TARGET_DIR) \
		COMPONENT_TYPE=$(CLICK_COMPONENT_TYPE) install
endef

$(eval $(generic-package))

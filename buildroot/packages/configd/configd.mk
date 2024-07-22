################################################################################
#
# configd
#
################################################################################

CONFIGD_VERSION = 0.1
CONFIGD_SITE = package/configd
CONFIGD_SITE_METHOD = local
CONFIGD_INSTALL_TARGET = YES
CONFIGD_DEPENDENCIES = json-c

define CONFIGD_BUILD_CMDS
$(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

define CONFIGD_INSTALL_TARGET_CMDS
$(INSTALL) -D -m 0755 $(@D)/configd $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
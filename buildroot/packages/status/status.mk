################################################################################
#
# status
#
################################################################################

STATUS_VERSION = 0.1
STATUS_SITE = package/status
STATUS_SITE_METHOD = local
STATUS_INSTALL_TARGET = YES
STATUS_DEPENDENCIES = json-c

define STATUS_BUILD_CMDS
$(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

define STATUS_INSTALL_TARGET_CMDS
$(INSTALL) -D -m 0755 $(@D)/status $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
################################################################################
#
# findhdr
#
################################################################################

FINDHDR_VERSION = 0.1
FINDHDR_SITE = package/findhdr
FINDHDR_SITE_METHOD = local
FINDHDR_INSTALL_TARGET = YES

define FINDHDR_BUILD_CMDS
$(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

define FINDHDR_INSTALL_TARGET_CMDS
$(INSTALL) -D -m 0755 $(@D)/find_hdr $(TARGET_DIR)/bin
endef

$(eval $(generic-package))

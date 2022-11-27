################################################################################
#
# pd690xx
#
################################################################################

PD690XX_VERSION = 0.41
PD690XX_SITE = package/pd690xx
PD690XX_SITE_METHOD = local
PD690XX_INSTALL_TARGET = YES

define PD690XX_BUILD_CMDS
$(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

define PD690XX_INSTALL_TARGET_CMDS
$(INSTALL) -D -m 0755 $(@D)/pd690xx $(TARGET_DIR)/bin
$(INSTALL) -D -m 0755 $(@D)/libpd690xx.so $(TARGET_DIR)/lib
endef

$(eval $(generic-package))

#ifndef libpd690xx
#define libpd690xx

// All values sourced from Auto Mode PD690xx Registers Map

// Timer Value for Overload conditions
#define TIMER_OVERLOAD_AT 0x100E
#define TIMER_OVERLOAD_AF 0x1010

// Maximum Vmain Voltage Level Threshold
#define VMAIN_HT 0x12FE
// Minimum Vmain Voltage Level Threshold
#define VMAIN_AT_LT 0x1300
#define VMAIN_AF_LT 0x1302

// Total power budget
#define MASTER_CFG_POWER_BASE 0x138C
#define PM_MODE_SYS_FLAG 0x1160 // bit 6

// General use
#define GENERAL_USE_REG 0x0318
#define SW_CFG_REG 0x139E
#define I2C_COMM_EXT_SYNC 0x1318
#define EXT_EVENT_INTR 0x1144

// System Status / Monitoring
#define VMAIN 0x105C
#define SYS_INIT 0x1164
#define AVG_JCT_TEMP 0x130A
#define CFGC_ICVER 0x031A
#define SYS_TOTAL_POWER 0x12E8
#define TOTAL_POWER_SLAVE_BASE 0x12EC
#define LOCAL_TOTAL_POWER 0x12AA
#define ADD_IC_STATUS 0x1314
#define UPD_POWER_MGMT_PARAMS 0x139C
#define SW_BOOT_STATE 0x1168

// Port status monitoring
#define PORT_CLASS_BASE 0x11C2
#define PORT_POWER_BASE 0x12B4
#define PORT_PM_INDICATION 0x129C // bit per port

// Port configuration
#define PORT_ICUT_MODE 0x1160 // bit 4
#define PORT_CR_BASE 0x131A
#define PORT_EN_CTL 0x1332 // bit per port
#define PORT_CURRENT_SENSE_BASE 0x1044

// Used by port_base_addr
#define PORT_CONFIG 1
#define PORT_POWER 2

// Used for the port status
#define PORT_DISABLED 0
#define PORT_ENABLED 1
#define PORT_FORCED 2
#define PORT_MODE_AF 0
#define PORT_MODE_AT 1
#define PORT_PRIO_CRIT 0
#define PORT_PRIO_HIGH 1
#define PORT_PRIO_LOW 2

#define MAX_PD690XX_COUNT 4

struct pd690xx_cfg {
    // Array of file descriptors used to talk to the I2C bus 1 and 2
    int i2c_fds[2];
    // pd690xx addresses
    unsigned char pd690xx_addrs[4];
    // detect if the pd690xx devices are present
    int pd690xx_pres[4];
};

void i2c_init(struct pd690xx_cfg *pd690xx);
void i2c_close(struct pd690xx_cfg *pd690xx);
int i2c_write(int, unsigned char, unsigned int, unsigned int);
int i2c_read(int, unsigned char, unsigned int, unsigned int *);
unsigned char get_pd690xx_addr(struct pd690xx_cfg *pd690xx, int);
int pd690xx_bus(struct pd690xx_cfg *pd690xx, int);
int pd690xx_pres_count(struct pd690xx_cfg *pd690xx);
unsigned int port_base_addr(int, int);
int port_able(struct pd690xx_cfg *pd690xx, int, int);
int port_enable(struct pd690xx_cfg *pd690xx, int);
int port_disable(struct pd690xx_cfg *pd690xx, int);
int port_reset(struct pd690xx_cfg *pd690xx, int);
int port_force(struct pd690xx_cfg *pd690xx, int);
int port_state(struct pd690xx_cfg *pd690xx, int);
int port_type(struct pd690xx_cfg *pd690xx, int);
int get_power(struct pd690xx_cfg *pd690xx, int);
int get_voltage(struct pd690xx_cfg *pd690xx);
int get_temp(struct pd690xx_cfg *pd690xx);
float port_power(struct pd690xx_cfg *pd690xx, int);
int port_priority(struct pd690xx_cfg *pd690xx, int);
void enable_debug(void);

#endif

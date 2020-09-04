#ifndef pd690xx
#define pd690xx
// gathered from I2C trace of libpoecore
#define PD690XX_I2C_ADDR 0x30

// All values sourced from Auto Mode PD690xx Registers Map

// Timer Value for Overload conditions
#define TIMER_AT 0x100E
#define TIMER_AF 0x1010

// Maximum Vmain Voltage Level Threshold
#define VMAIN_HT 0x12FE
// Minimum Vmain Voltage Level Threshold
#define VMAIN_AT_LT 0x1300
#define VMAIN_AF_LT 0x1302

// System Status / Monitoring
#define VMAIN 0x105C
#define AVG_JCT_TEMP 0x130A
#define SYS_TOTAL_POWER 0x12E8
#define TOTAL_POWER_SLAVE_BASE 0x12EC
#define LOCAL_TOTAL_POWER 0x12AA
#define UPD_POWER_MGMT_PARAMS 0x139C

// Port status monitoring
#define PORT_POWER_BASE 0x12B4

// Port configuration
#define PORT_CR_BASE 0x131A

#endif

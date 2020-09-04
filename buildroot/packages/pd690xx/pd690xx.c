#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <linux/i2c-dev.h>
// Terrible portability hack between arm-linux-gnueabihf-gcc on Mac OS X and native gcc on raspbian.
#ifndef I2C_M_RD
#include <linux/i2c.h>
#endif

// getopt
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

// register map
#include "pd690xx.h"

// adapted from
// https://gist.github.com/JamesDunne/9b7fbedb74c22ccc833059623f47beb7

typedef unsigned char   u8;
typedef unsigned int   u16;

// Global file descriptor used to talk to the I2C bus:
int i2c_fd = -1;
// Default RPi B device name for the I2C bus exposed on GPIO2,3 pins (GPIO2=SDA, GPIO3=SCL): 
const char *i2c_fname = "/dev/i2c-1";

// Returns a new file descriptor for communicating with the I2C bus:
int i2c_init(void) {
    if ((i2c_fd = open(i2c_fname, O_RDWR)) < 0) {
        char err[200];
        sprintf(err, "open('%s') in i2c_init", i2c_fname);
        perror(err);
        return -1;
    }

    // NOTE we do not call ioctl with I2C_SLAVE here because we always use the I2C_RDWR ioctl operation to do
    // writes, reads, and combined write-reads. I2C_SLAVE would be used to set the I2C slave address to communicate
    // with. With I2C_RDWR operation, you specify the slave address every time. There is no need to use normal write()
    // or read() syscalls with an I2C device which does not support SMBUS protocol. I2C_RDWR is much better especially
    // for reading device registers which requires a write first before reading the response.

    return i2c_fd;
}

void i2c_close(void) {
    close(i2c_fd);
}

// Write to an I2C slave device's register:
int i2c_write(u8 slave_addr, u16 reg, u16 data) {
    int retval;
    u8 outbuf[4];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = (reg >> 8) & 0xFF;
    outbuf[1] = reg & 0xFF;
    outbuf[2] = (data >> 8) & 0xFF;
    outbuf[3] = data & 0xFF;

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 4;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_write");
        return -1;
    }

    return 0;
}

// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(u8 slave_addr, u16 reg, u16 *result) {
    int retval;
    u8 outbuf[2], inbuf[2];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 2;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = (reg >> 8) & 0xFF;
    outbuf[1] = reg & 0xFF;

    inbuf[0] = 0;
    inbuf[1] = 0;

    *result = 0;
    if (ioctl(i2c_fd, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_read");
        return -1;
    }

    // debug: print raw I2C response as hex
    printf("I2C data: %02X%02X\n", inbuf[0], inbuf[1]);
    *result = (u16)inbuf[0] << 8 | (u16)inbuf[1];
    return 0;
}

unsigned int port_cr_addr(int port) {
    u16 port_base = PORT_CR_BASE;
    // PORT_CR_BASE is 0x131A which is for PORT0
    return (port_base-2)+(unsigned int)(port*2);
}

int port_disable(int port) {
    if (port > 11) {
        printf("Invalid port number: %d\n", port);
	return -1;
    }
    u16 res;
    u16 port_addr;
    port_addr = port_cr_addr(port);
    //printf("Port addr: %02X\n", port_addr);
    // disable the port
    res = i2c_write(PD690XX_I2C_ADDR, port_addr, 0x00);
    if (res != 0) {
        printf("Error disabling port %d\n", port);
	return -1;
    }
    // sleep before we poll the port register
    usleep(100000);
    i2c_read(PD690XX_I2C_ADDR, port_addr, &res);
    if ((res & 0x03) == 0) {
        printf("Port %d PoE disabled\n", port);
	return 0;
    }
    return -1;
}

int port_enable(int port) {
    if (port > 11) {
        printf("Invalid port number: %d\n", port);
	return -1;
    }
    u16 res;
    u16 port_addr = port_cr_addr(port);
    //printf("Port addr: %02X\n", port_addr);
    // disable the port
    res = i2c_write(PD690XX_I2C_ADDR, port_addr, 0x01);
    if (res != 0) {
        printf("Error enabling port %d\n", port);
	return -1;
    }
    // sleep before we poll the port register
    usleep(100000);
    i2c_read(PD690XX_I2C_ADDR, port_addr, &res);
    if ((res & 0x03) == 1) {
        printf("Port %d PoE enabled\n", port);
	return 0;
    }
    return -1;
}

int get_temp(void) {
    u16 res;
    int temperature;
    i2c_read(PD690XX_I2C_ADDR, AVG_JCT_TEMP, &res);
    temperature = (res / -1.514)-40;
    printf("%d\n");
    return 0;
}

int get_power(void) {
    u16 res;
    i2c_write(PD690XX_I2C_ADDR, UPD_POWER_MGMT_PARAMS, 0x01);
    i2c_read(PD690XX_I2C_ADDR, SYS_TOTAL_POWER, &res);
    printf("%.1f W\n", (float)res/10);
}

int main (int argc, char **argv) {
    u16 res;
    u16 bus;
    u16 fdisable;
    u16 fenable;
    u16 ftemp;
    int port = -1;
    int c;
    // https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Example-of-Getopt.html
    while (( c = getopt (argc, argv, "d:e:tpb:")) != -1) {
      switch(c) {
        case 'b':
          bus = atoi(optarg);
	  break;
        case 'd':
          res = i2c_init();
	  port_disable(atoi(optarg));
	  break;
	case 'e':
          res = i2c_init();
	  port_enable(atoi(optarg));
	  break;
	case 'p':
          res = i2c_init();
	  get_power();
	  break;
	case 't':
          res = i2c_init();
	  get_temp();
	  break;
	case '?':
	  return 1;
	default:
	  return 0;
      }
    }

    // close the file descriptor when exiting
    i2c_close();
}

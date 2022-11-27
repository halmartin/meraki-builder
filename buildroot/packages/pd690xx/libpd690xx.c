#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h> 

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

// getopt
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

// basename
#include <libgen.h>

// register map
#include "libpd690xx.h"

// Copyright(C) 2020-2022 - Hal Martin <hal.martin@gmail.com>

// portions adapted from
// https://gist.github.com/JamesDunne/9b7fbedb74c22ccc833059623f47beb7

// define the static part of the file path
const char *i2c_fname_base = "/dev/i2c-";
char i2c_fname[11];

char DEBUG = 0;

void enable_debug(void) {
    DEBUG = 1;
}

void i2c_init(struct pd690xx_cfg *pd690xx) {
    int i2c_fd;
    int start_idx = 0;
    for (int i2c_dev=0; i2c_dev<1; i2c_dev++) {
      snprintf(i2c_fname, sizeof(i2c_fname), "%s%d", i2c_fname_base, i2c_dev+1);
      if ((i2c_fd = open(i2c_fname, O_RDWR)) < 0) {
        // couldn't open the device
        pd690xx->i2c_fds[i2c_dev] = -1;
      } else {
        // save the file descriptor in the array of global file descriptors
        pd690xx->i2c_fds[i2c_dev] = i2c_fd;
        unsigned int res;

        if (i2c_dev == 1) {
            start_idx = 2;
        }

        if (DEBUG) {
            fprintf(stderr, "/dev/i2c-%d\n", i2c_dev);
        }
        for (int i=start_idx; i<4; i++) {
            // read the CFGC_ICVER register and if we get a response
            // mark the pd690xx as present
            if (DEBUG) {
                fprintf(stderr, "Probing I2C address %02X\n", pd690xx->pd690xx_addrs[i]);
            }
            i2c_read(i2c_fd, pd690xx->pd690xx_addrs[i], CFGC_ICVER, &res);
            if ((res >> 10) > 0) {
                pd690xx->pd690xx_pres[i] = 1;
            }
        }
    }
    if (DEBUG) {
        fprintf(stderr, "Detected %d pd690xx\n", pd690xx_pres_count(pd690xx));
    }
    }
}

void i2c_close(struct pd690xx_cfg *pd690xx) {
    // loop through i2c-1 and i2c-2 and close them if they were open
    for (int i=0; i<2; i++){
        if (pd690xx->i2c_fds[i] != -1) {
            close(pd690xx->i2c_fds[i]);
        }
    }
}

// Write to an I2C slave device's register:
int i2c_write(int i2c_fd, unsigned char slave_addr, unsigned int reg, unsigned int data) {
    unsigned char outbuf[4];

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
        // we're going to relegate this to debug
        // since it is expected to fire during pd690xx detection
        if (DEBUG) {
            perror("ioctl(I2C_RDWR) in i2c_write");
        }
        return -1;
    }

    return 0;
}

// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(int i2c_fd, unsigned char slave_addr, unsigned int reg, unsigned int *result) {
    unsigned char outbuf[2], inbuf[2];
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
        // we're going to relegate this to debug
        // since it is expected to fire during pd690xx detection
        if (DEBUG) {
            perror("ioctl(I2C_RDWR) in i2c_read");
        }
        return -1;
    }

    if (DEBUG) {
        // print raw I2C response as hex
        // otherwise you can always use kernel i2c tracing
        fprintf(stderr, "I2C data: %02X%02X\n", inbuf[0], inbuf[1]);
    }
    *result = (unsigned int)inbuf[0] << 8 | (unsigned int)inbuf[1];
    return 0;
}

unsigned char get_pd690xx_addr(struct pd690xx_cfg *pd690xx, int port) {
    // check the port number AND verify that the pd690xx is present
    // on the bus before returning the address
    if (port > 0) {
        port = port-1;
    }
    int select = abs(port/12);
    // shouldn't have more than 4 (MAX_PD690XX_COUNT) pd690xx in the switch
    // port number is too high
    if (select > MAX_PD690XX_COUNT) {
        return 0;
    }
    switch(pd690xx->pd690xx_pres[select]) {
        case 0:
            // selected pd690xx is not present
            return 0;
            break;
        case 1:
            // pd690xx is present, return I2C address
            return pd690xx->pd690xx_addrs[select];
            break;
    }
    return 0;
}

int pd690xx_fd(struct pd690xx_cfg *pd690xx, int port) {
    switch(port/12) {
        case 0:
        case 1:
        case 2:
        case 3:
        default:
            return pd690xx->i2c_fds[0];
            break;
    }
}

int pd690xx_pres_count(struct pd690xx_cfg *pd690xx) {
    int total = 0;
    for (int i=0; i<4; i++) {
        if (pd690xx->pd690xx_pres[i] == 1) {
            total++;
        }
    }
    return total;
}

unsigned int port_base_addr(int type, int port) {
    unsigned int port_base;
    switch(type) {
        case PORT_CONFIG:
            // PORT_CR_BASE is 0x131A which is for PORT0
            port_base = PORT_CR_BASE;
            break;
        case PORT_POWER:
            port_base = PORT_POWER_BASE;
            break;
        default:
            port_base = PORT_CR_BASE;
    }
    // sanity check the port number
    if (port < 1) {
        port = 1;
    }
    // ensure that we only operate on ports < 12
    // or we calculate the wrong port address
    return port_base+(unsigned int)(((port-1)%12)*2);
}

int port_able(struct pd690xx_cfg *pd690xx, int op, int port) {
    const char *operation[4] = {"disabl", "enabl", "forc", "reserv"};
    unsigned int res;
    unsigned int reg;
    unsigned char pd_addr = get_pd690xx_addr(pd690xx, port);
    if (pd_addr == 0) {
        return -1;
    }
    unsigned int port_addr = port_base_addr(PORT_CONFIG, port);
    if (DEBUG) {
        fprintf(stderr, "Port addr: %02X\n", port_addr);
    }
    int i2c_fd = pd690xx_fd(pd690xx, port);
    i2c_read(i2c_fd, pd_addr, port_addr, &reg);
    switch(op) {
        case PORT_DISABLED:
            res = i2c_write(i2c_fd, pd_addr, port_addr, reg & 0xFC);
            break;
        case PORT_ENABLED:
            res = i2c_write(i2c_fd, pd_addr, port_addr, (reg & 0xFD) | 0x01);
            break;
        case PORT_FORCED:
            res = i2c_write(i2c_fd, pd_addr, port_addr, (reg & 0xFC) | 0x02);
            break;
    }
    if (res != 0) {
        printf("Error %sing port %d\n", operation[op], port);
        return -1;
    }
    // sleep before we poll the port register
    usleep(100000);
    i2c_read(i2c_fd, pd_addr, port_addr, &reg);
    printf("Port %d: %sed\n", port, operation[(reg & 0x03)]);
    return 0;
}

int port_disable(struct pd690xx_cfg *pd690xx, int port) {
    // only toggle the port if desired != present state
    if (port_state(pd690xx, port) != PORT_DISABLED) {
        if (DEBUG) {
            fprintf(stderr, "Disabling port %d\n", port);
        }
        return port_able(pd690xx, PORT_DISABLED, port);
    } else {
        if (DEBUG) {
            fprintf(stderr, "Port %d already disabled\n", port);
        }
    }
    return 0;
}

int port_enable(struct pd690xx_cfg *pd690xx, int port) {
    // only toggle the port if desired != present state
    if (port_state(pd690xx, port) != PORT_ENABLED) {
        if (DEBUG) {
            fprintf(stderr, "Enabling port %d\n", port);
        }
        return port_able(pd690xx, PORT_ENABLED, port);
    } else {
        if (DEBUG) {
            fprintf(stderr, "Port %d already enabled\n", port);
        }
    }
    return 0;
}

int port_force(struct pd690xx_cfg *pd690xx, int port) {
    // only toggle the port if desired != present state
    if (port_state(pd690xx, port) != PORT_FORCED) {
        if (DEBUG) {
            fprintf(stderr, "Forcing port %d\n", port);
        }
        return port_able(pd690xx, PORT_FORCED, port);
    } else {
        if (DEBUG) {
            fprintf(stderr, "Port %d already forced\n", port);
        }
    }
    return 0;
}


int port_reset(struct pd690xx_cfg *pd690xx, int port) {
    port_disable(pd690xx, port);
    usleep(2000000);
    port_enable(pd690xx, port);
    return 0;
}

float port_power(struct pd690xx_cfg *pd690xx, int port) {
    unsigned int res;
    unsigned char pd_addr = get_pd690xx_addr(pd690xx, port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(pd690xx, port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_POWER, port), &res);
    return (float)res/10;
}

int port_state(struct pd690xx_cfg *pd690xx, int port) {
    unsigned int res;
    int port_enabled = -1;
    unsigned char pd_addr = get_pd690xx_addr(pd690xx, port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(pd690xx, port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_CONFIG, port), &res);
    port_enabled = res & 0x03;
    if (DEBUG) {
        switch(port_enabled) {
            case PORT_DISABLED:
                fprintf(stderr, "Port %d: disabled\n", port);
                break;
            case PORT_ENABLED:
                fprintf(stderr, "Port %d: enabled\n", port);
                break;
            case PORT_FORCED:
                fprintf(stderr, "Port %d: forced\n", port);
                break;
            default:
                fprintf(stderr, "Port %d: unknown\n", port);
        }
    }
    return port_enabled;
}

int port_type(struct pd690xx_cfg *pd690xx, int port) {
    unsigned int res;
    int port_mode = -1;
    unsigned char pd_addr = get_pd690xx_addr(pd690xx, port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(pd690xx, port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_CONFIG, port), &res);
    port_mode = (res & 0x30) >> 4;
    if (DEBUG) {
        switch(port_mode) {
            case PORT_MODE_AF:
                fprintf(stderr, "Port %d: 802.3af\n", port);
                break;
            case PORT_MODE_AT:
                fprintf(stderr, "Port %d: 802.3at\n", port);
                break;
            default:
                fprintf(stderr, "Port %d: unknown\n", port);
        }
    }
    return port_mode;
}

int port_priority(struct pd690xx_cfg *pd690xx, int port) {
    unsigned int res;
    int port_prio = -1;
    unsigned char pd_addr = get_pd690xx_addr(pd690xx, port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(pd690xx, port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_CONFIG, port), &res);
    port_prio = res & 0xC0;
    if (DEBUG) {
        switch(port_prio) {
            case PORT_PRIO_CRIT:
                fprintf(stderr, "Port %d: Critical\n", port);
                break;
            case PORT_PRIO_HIGH:
                fprintf(stderr, "Port %d: High\n", port);
                break;
            case PORT_PRIO_LOW:
                fprintf(stderr, "Port %d: Low\n", port);
                break;
            default:
                fprintf(stderr, "Port %d: unknown\n", port);
        }
    }
    return port_prio;
}

int get_temp(struct pd690xx_cfg *pd690xx) {
    unsigned int res;
    int pd690xx_count = pd690xx_pres_count(pd690xx);
    for (int i=0; i<pd690xx_count; i++) {
        int i2c_fd = pd690xx_fd(pd690xx, i*12);
        i2c_read(i2c_fd, pd690xx->pd690xx_addrs[i], AVG_JCT_TEMP, &res);
        printf("%.1f C\n", (((int)res-684)/-1.514)-40);
    }
    return 0;
}

int get_power(struct pd690xx_cfg *pd690xx, int port) {
    unsigned int res;
    // only -p was specified, get system power
    if (port == 0) {
        float total = 0;
        int pd690xx_count = pd690xx_pres_count(pd690xx);
        for (int i=0; i<pd690xx_count; i++) {
            int i2c_fd = pd690xx_fd(pd690xx, i*12);
            i2c_write(i2c_fd, pd690xx->pd690xx_addrs[i], UPD_POWER_MGMT_PARAMS, 0x01);
            usleep(100000);
            i2c_read(i2c_fd, pd690xx->pd690xx_addrs[i], SYS_TOTAL_POWER, &res);
            total += (float)res/10;
        }
        printf("Total: %.1f W\n", total);
    } else {
        // optarg was a port number, so get power for a specific port
        float power = port_power(pd690xx, port);
        printf("Port %d: %.1f W\n", port, power);
    }
    return 0;
}

int get_voltage(struct pd690xx_cfg *pd690xx) {
    unsigned int res;
    int pd690xx_count = pd690xx_pres_count(pd690xx);
    for (int i=0; i<pd690xx_count; i++) {
        int i2c_fd = pd690xx_fd(pd690xx, i*12);
        i2c_read(i2c_fd, pd690xx->pd690xx_addrs[i], VMAIN, &res);
        printf("%.1f V\n", (float)(res*0.061));
    } 
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

// getopt
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

// basename
#include <libgen.h>

// register map
#include "pd690xx.h"

// Copyright(C) 2020 - Hal Martin <hal.martin@gmail.com>

// portions adapted from
// https://gist.github.com/JamesDunne/9b7fbedb74c22ccc833059623f47beb7

typedef unsigned char   u8;
typedef unsigned int   u16;

// Global array of file descriptors used to talk to the I2C bus 1 and 2
int i2c_fds[2] = {-1, -1};
// hold pd690xx addresses
u8 pd690xx_addrs[4] = {PD690XX0_I2C_ADDR, PD690XX1_I2C_ADDR, PD690XX2_I2C_ADDR, PD690XX3_I2C_ADDR};
// detect if the pd690xx devices are present
int pd690xx_pres[4] = {0,0,0,0};
// define the static part of the file path
const char *i2c_fname_base = "/dev/i2c-";
char i2c_fname[11];

int i2c_init(void) {
    int i2c_fd;
    for (int i2c_dev=0; i2c_dev<1; i2c_dev++) {
      snprintf(i2c_fname, sizeof(i2c_fname), "%s%d", i2c_fname_base, i2c_dev+1);
      if ((i2c_fd = open(i2c_fname, O_RDWR)) < 0) {
        // couldn't open the device
        i2c_fds[i2c_dev] = -1;
      } else {
        // save the file descriptor in the array of global file descriptors
        i2c_fds[i2c_dev] = i2c_fd;
        u16 res;
        switch (i2c_dev) {
            case 0:
                #ifdef DEBUG
                    printf("/dev/i2c-0\n");
                #endif
                for (int i=0; i<4; i++) {
                    // read the CFGC_ICVER register and if we get a response
                    // mark the pd690xx as present
                    #ifdef DEBUG
                        printf("Probing I2C address %02X\n", pd690xx_addrs[i]);
                    #endif
                    i2c_read(i2c_fd, pd690xx_addrs[i], CFGC_ICVER, &res);
                    if ((res >> 10) > 0) {
                        pd690xx_pres[i] = 1;
                    }
                }
                break;
            case 1:
                #ifdef DEBUG
                    printf("/dev/i2c-1\n");
                #endif
                for (int i=2; i<4; i++) {
                    #ifdef DEBUG
                        printf("Probing I2C address %02X\n", pd690xx_addrs[i]);
                    #endif
                    i2c_read(i2c_fd, pd690xx_addrs[i], CFGC_ICVER, &res);
                    if ((res >> 10) > 0) {
                        pd690xx_pres[i] = 1;
                    }
                }
                break;
        }
      }
    }
    #ifdef DEBUG
        printf("Detected %d pd690xx\n", pd690xx_pres_count());
    #endif
}

void i2c_close() {
    // loop through i2c-1 and i2c-2 and close them if they were open
    for (int i=0; i<2; i++){
        if (i2c_fds[i] != -1) {
            close(i2c_fds[i]);
        }
    }
}

// Write to an I2C slave device's register:
int i2c_write(int i2c_fd, u8 slave_addr, u16 reg, u16 data) {
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
        // we're going to relegate this to debug
        // since it is expected to fire during pd690xx detection
        #ifdef DEBUG
            perror("ioctl(I2C_RDWR) in i2c_write");
        #endif
        return -1;
    }

    return 0;
}

// Read the given I2C slave device's register and return the read value in `*result`:
int i2c_read(int i2c_fd, u8 slave_addr, u16 reg, u16 *result) {
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
        // we're going to relegate this to debug
        // since it is expected to fire during pd690xx detection
        #ifdef DEBUG
            perror("ioctl(I2C_RDWR) in i2c_read");
        #endif
        return -1;
    }

    #ifdef DEBUG
        // print raw I2C response as hex
        // otherwise you can always use kernel i2c tracing
        printf("I2C data: %02X%02X\n", inbuf[0], inbuf[1]);
    #endif
    *result = (u16)inbuf[0] << 8 | (u16)inbuf[1];
    return 0;
}

unsigned char get_pd690xx_addr(int port) {
    // check the port number AND verify that the pd690xx is present
    // on the bus before returning the address
    int select = abs(port/12);
    // can't have more than 4 pd690xx in the switch
    // port number is too high
    if (select > 3) {
        return 0;
    }
    switch(pd690xx_pres[select]) {
        case 0:
            // selected pd690xx is not present
            return 0;
            break;
        case 1:
            // pd690xx is present, return I2C address
            return pd690xx_addrs[select];
            break;
    }
    // if (0 < port < 12) && (pd690xx_pres[0] == 1) {
    //     return PD690XX0_I2C_ADDR;
    // } elif (12 < port < 24) && (pd690xx_pres[1] == 1) {
    //     return PD690XX1_I2C_ADDR;
    // } elif (24 < port < 36) && (pd690xx_pres[2] == 1) {
    //     return PD690XX2_I2C_ADDR;
    // } elif (36 < port < 48) && (pd690xx_pres[3] == 1) {
    //     return PD690XX3_I2C_ADDR;
    // }
    // port number was bogus or pd690xx was not present on bus
    return 0;
}

int pd690xx_fd(int port) {
    switch(port/12) {
        case 0:
        case 1:
        case 2:
        case 3:
        default:
            return i2c_fds[0];
            break;
    }
}

int pd690xx_pres_count(void) {
    int total = 0;
    for (int i=0; i<4; i++) {
        if (pd690xx_pres[i] == 1) {
            total++;
        }
    }
    return total;
}

unsigned int port_base_addr(int type, int port) {
    u16 port_base;
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
    return (port_base-2)+(unsigned int)(port*2);
}

int port_able(int op, int port) {
    const char *operation[4] = {"disabl", "enabl", "forc", "reserv"};
    u16 res;
    u16 reg;
    u8 pd_addr = get_pd690xx_addr(port);
    if (pd_addr == 0) {
        return -1;
    }
    u16 port_addr = port_base_addr(PORT_CONFIG, port);
    #ifdef DEBUG
        printf("Port addr: %02X\n", port_addr);
    #endif
    int i2c_fd = pd690xx_fd(port);
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

int port_disable(int port) {
    return port_able(PORT_DISABLED, port);
}

int port_enable(int port) {
    return port_able(PORT_ENABLED, port);
}

int port_force(int port) {
    return port_able(PORT_FORCED, port);
}


int port_reset(int port) {
    port_disable(port);
    usleep(2000000);
    port_enable(port);
    return 0;
}

float port_power(int port) {
    u16 res;
    u8 pd_addr = get_pd690xx_addr(port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_POWER, port), &res);
    return (float)res/10;
}

int port_state(int port) {
    u16 res;
    int port_enabled = -1;
    u8 pd_addr = get_pd690xx_addr(port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_CONFIG, port), &res);
    port_enabled = res & 0x03;
    #ifdef DEBUG
    switch(port_enabled) {
        case PORT_DISABLED:
            printf("Port %d: disabled\n", port);
            break;
        case PORT_ENABLED:
            printf("Port %d: enabled\n", port);
            break;
        case PORT_FORCED:
            printf("Port %d: forced\n", port);
            break;
        default:
            printf("Port %d: unknown\n", port);
    }
    #endif
    return port_enabled;
}

int port_type(int port) {
    u16 res;
    int port_mode = -1;
    u8 pd_addr = get_pd690xx_addr(port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_CONFIG, port), &res);
    port_mode = (res & 0x30) >> 4;
    #ifdef DEBUG
    switch(port_mode) {
        case PORT_MODE_AF:
            printf("Port %d: 802.3af\n", port);
            break;
        case PORT_MODE_AT:
            printf("Port %d: 802.3at\n", port);
            break;
        default:
            printf("Port %d: unknown\n", port);
    }
    #endif
    return port_mode;
}

int port_priority(int port) {
    u16 res;
    int port_prio = -1;
    u8 pd_addr = get_pd690xx_addr(port);
    if (pd_addr == 0) {
        return -1;
    }
    int i2c_fd = pd690xx_fd(port);
    i2c_read(i2c_fd, pd_addr, port_base_addr(PORT_CONFIG, port), &res);
    port_prio = res & 0xC0;
    #ifdef DEBUG
    switch(port_prio) {
        case PORT_PRIO_CRIT:
            printf("Port %d: Critical\n", port);
            break;
        case PORT_PRIO_HIGH:
            printf("Port %d: High\n", port);
            break;
        case PORT_PRIO_LOW:
            printf("Port %d: Low\n", port);
            break;
        default:
            printf("Port %d: unknown\n", port);
    }
    #endif
    return port_prio;
}

int get_temp(void) {
    u16 res;
    int pd690xx_count = pd690xx_pres_count();
    // this seems _kind of_ sane, but I'm not sure
    // the datasheet has two different versions of the formula
    // and nothing about a sign bit, since it's -200 to 400 C
    for (int i=0; i<pd690xx_count; i++) {
        i2c_read(pd690xx_fd(i*12), pd690xx_addrs[i], AVG_JCT_TEMP, &res);
        printf("%.1f C\n", (((int)res-684)/-1.514)-40);
    }
    return 0;
}

int get_power(int port) {
    u16 res;
    // only -p was specified, get system power
    if (port == 0) {
        float total = 0;
        int pd690xx_count = pd690xx_pres_count();
        for (int i=0; i<pd690xx_count; i++) {
            i2c_write(pd690xx_fd(i*12), pd690xx_addrs[i], UPD_POWER_MGMT_PARAMS, 0x01);
            usleep(100000);
            i2c_read(pd690xx_fd(i*12), pd690xx_addrs[i], SYS_TOTAL_POWER, &res);
            total += (float)res/10;
        }
        printf("Total: %.1f W\n", total);
    } else {
        // optarg was a port number, so get power for a specific port
        float power = port_power(port);
        printf("Port %d: %.1f W\n", port, power);
    }
    return 0;
}

int get_voltage() {
    u16 res;
    float total = 0;
    int pd690xx_count = pd690xx_pres_count();
    for (int i=0; i<pd690xx_count; i++) {
        i2c_read(pd690xx_fd(i*12), pd690xx_addrs[i], VMAIN, &res);
        printf("%.1f V\n", (float)(res*0.061));
    } 
    return 0;
}

void list_all() {
    u16 res;
    printf("Port\tStatus\t\tType\tPriority\tPower\n");
    int total_ports = 12*pd690xx_pres_count();
    for (int i=1; i<total_ports; i++) {
        // print port number
        printf("%d\t", i);
        // print port status
        switch(port_state(i)) {
            case PORT_DISABLED:
                printf("Disabled\t");
                break;
            case PORT_ENABLED:
                printf("Enabled\t\t");
                break;
            case PORT_FORCED:
                printf("Forced\t\t");
                break;
            default:
                printf("Unknown\t");
        }
        // print port type
        switch(port_type(i)) {
            case PORT_MODE_AF:
                printf("af\t");
                break;
            case PORT_MODE_AT:
                printf("at\t");
                break;
            default:
                printf("Unknown\t");
        }
        // print port priority
        switch(port_priority(i)) {
            case PORT_PRIO_CRIT:
                printf("Critical\t");
                break;
            case PORT_PRIO_HIGH:
                printf("High\t\t");
                break;
            case PORT_PRIO_LOW:
                printf("Low\t\t");
                break;
            default:
                printf("Unknown\t");
        }
        // print port power
        printf("%.1f\n", port_power(i));
    }
}

void usage(char **argv) {
    // there are SO discussions about whether to bake in the name
    // or get it from the executable path, for now, use the basename
    char* binary = basename(argv[0]);
    printf("Usage: %s [OPTIONS]\n", binary);
    printf("Options:\n");
    printf("\t-d <PORT>\tDisable PoE on port PORT\n");
    printf("\t-e <PORT>\tEnable PoE on port PORT\n");
    printf("\t-f <PORT>\tForce PoE on port PORT\n");
    printf("\t-h\t\tProgram usage\n");
    printf("\t-l\t\tList all port statuses\n");
    printf("\t-p [PORT]\tPrint PoE power consumption (system total, or on port PORT)\n");
    printf("\t-r <PORT>\tReset PoE on port PORT\n");
    printf("\t-s\t\tDisplay PoE voltage\n");
    printf("\t-t\t\tDisplay average junction temperature (deg C) of pd690xx\n");
}

int main (int argc, char **argv) {
    int port = -1;
    int ret_val = 0;
    int c;
    // https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Example-of-Getopt.html
    if (argc == 1) {
        usage(argv);
        return 0;
    }
    while (( c = getopt (argc, argv, "htsld:e:f:r:p::")) != -1) {
      switch(c) {
        case 'd':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              port_disable(atoi(optarg));
          } else {
              ret_val = 1;
          }
          break;
        case 'e':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              port_enable(atoi(optarg));
          } else {
              ret_val = 1;
          }
          break;
        case 'f':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              port_force(atoi(optarg));
          } else {
              ret_val = 1;
          }
          break;
        case 'l':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              list_all();
          } else {
              ret_val = 1;
          }
          break;
        case 'p':
          if (argv[optind] == NULL) {
              port = 0;
          } else {
              // optarg doesn't seem to work if the
              // option has an optional value
              // dirty workaround
              port = atoi(argv[optind]);
          }
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              get_power(port);
          } else {
              ret_val = 1;
          }
          break;
        case 'r':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              port_reset(atoi(optarg));
          } else {
              ret_val = 1;
          }
          break;
        case 's':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              get_voltage();
          } else {
              ret_val = 1;
          }
          break;
        case 't':
          i2c_init();
          if (pd690xx_pres_count() > 0) {
              get_temp();
          } else {
              ret_val = 1;
          }
          break;
        case 'h':
        case '?':
          usage(argv);
          break;
        default:
          usage(argv);
          ret_val = 1;
          break;
      }
    }

    // close the file descriptor when exiting
    i2c_close();
    return ret_val;
}

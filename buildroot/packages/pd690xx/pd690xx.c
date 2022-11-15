#include <stdio.h>
#include <string.h>
#include <fcntl.h>

// getopt
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

// basename
#include <libgen.h>

// register map
#include "pd690xx_meraki.h"
#include "libpd690xx.h"

// Copyright(C) 2020-2022 - Hal Martin <hal.martin@gmail.com>

void list_all(struct pd690xx_cfg *pd690xx) {
    printf("Port\tStatus\t\tType\tPriority\tPower\n");
    int total_ports = 12*pd690xx_pres_count(pd690xx);
    for (int i=1; i<=total_ports; i++) {
        // print port number
        printf("%d\t", i);
        // print port status
        switch(port_state(pd690xx, i)) {
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
        switch(port_type(pd690xx, i)) {
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
        switch(port_priority(pd690xx, i)) {
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
        printf("%.1f\n", port_power(pd690xx, i));
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
    int c;
    char disable = 0, enable = 0, force= 0, list = 0, power = 0, reset = 0, voltage = 0, temp = 0, debug = 0;

    // https://www.gnu.org/savannah-checkouts/gnu/libc/manual/html_node/Example-of-Getopt.html
    if (argc == 1) {
        usage(argv);
        return 0;
    }
    while (( c = getopt (argc, argv, "htsld:e:f:r:p:v::")) != -1) {
      switch(c) {
        case 'd':
          disable = 1;
          port = atoi(optarg);
          break;
        case 'e':
          enable = 1;
          port = atoi(optarg);
          break;
        case 'f':
          force = 1;
          port = atoi(optarg);
          break;
        case 'l':
          list = 1;
          break;
        case 'p':
          power = 1;
          if (argv[optind] == NULL) {
              port = 0;
          } else {
              // optarg doesn't seem to work if the
              // option has an optional value
              // dirty workaround
              port = atoi(argv[optind]);
          }
          break;
        case 'r':
          reset = 1;
          port = atoi(optarg);
          break;
        case 's':
          voltage = 1;
          break;
        case 't':
          temp = 1;
          break;
        case 'v':
          debug = 1;
          break;
        case 'h':
        case '?':
          usage(argv);
          break;
        default:
          usage(argv);
          break;
      }
    }

    struct pd690xx_cfg pd690xx = {
        // i2c_fds
        {-1, -1},
        // pd690xx_addrs
        {PD690XX0_I2C_ADDR, PD690XX1_I2C_ADDR, PD690XX2_I2C_ADDR, PD690XX3_I2C_ADDR},
        // pd690xx_pres
        {0, 0, 0 ,0}
    };

    // first, enable debugging if requested
    if (debug) {
        enable_debug();
    }

    i2c_init(&pd690xx);

    // check if we detected any pd690xx present
    if (pd690xx_pres_count(&pd690xx) == 0) {
        // we didn't, so exit with an error
        return 1;
    }

    if (disable) {
        if (port < 1 || port > 48) { return 1; };
        port_disable(&pd690xx, port);
    }
    if (enable) {
        if (port < 1 || port > 48) { return 1; };
        port_enable(&pd690xx, port);
    }
    if (force) {
        if (port < 1 || port > 48) { return 1; };
        port_force(&pd690xx, port);
    }
    if (reset) {
        if (port < 1 || port > 48) { return 1; };
        port_reset(&pd690xx, port);
    }
    if (list) {
        list_all(&pd690xx);
    }
    if (power) {
        if (port < 1 || port > 48) { return 1; };
        get_power(&pd690xx, port);
    }
    if (voltage) {
        get_voltage(&pd690xx);
    }
    if (temp) {
        get_temp(&pd690xx);
    }

    i2c_close(&pd690xx);
}

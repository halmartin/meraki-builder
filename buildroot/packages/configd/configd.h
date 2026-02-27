#ifndef CONFIGD_H
#define CONFIGD_H

#include <libpd690xx.h>
#include <stdbool.h>

// shared global state
extern bool poe_capable;
extern bool dry_run;
extern struct pd690xx_cfg pd690xx;
extern char *config_file;
extern char meraki_mac[18];

#endif

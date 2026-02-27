#include "configd.h"
#include "status.h"
#include "click_port.h"
#include "click_global.h"
#include "json_util.h"
#include "config_file.h"
#include "websocket.h"

#include <libpostmerkos.h>
#include <libpd690xx.h>
#include "pd690xx_meraki.h"

#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// global state definitions
bool poe_capable = false;
bool dry_run = false;
char *config_file = "/etc/switch.json";
char meraki_mac[18] = "";

struct pd690xx_cfg pd690xx = {
    // i2c_fds
    {-1, -1},
    // pd690xx_addrs
    {PD690XX0_I2C_ADDR, PD690XX1_I2C_ADDR, PD690XX2_I2C_ADDR, PD690XX3_I2C_ADDR},
    // pd690xx_pres
    {0, 0, 0 ,0}
};

static volatile int running = 1;

static void sighandler(int sig) {
  running = 0;
}

int main(int argc, char **argv) {

  int ws_port = 4001;
  int status_interval = 3;
  int c;

  while ((c = getopt(argc, argv, "c:dp:w:")) != -1) {
    switch (c) {
    case 'c':
      config_file = optarg;
      break;
    case 'd':
      dry_run = true;
      break;
    case 'p':
      status_interval = atoi(optarg);
      break;
    case 'w':
      ws_port = atoi(optarg);
      break;
    }
  }

  // determine whether board is poe capable
  i2c_init(&pd690xx);
  if (pd690xx_pres_count(&pd690xx)) {
    poe_capable = true;
  }

  // read MAC address for STP port format
  FILE *macf = fopen("/tmp/MERAKI_MAC", "r");
  if (macf) {
    if (fgets(meraki_mac, sizeof(meraki_mac), macf)) {
      meraki_mac[strcspn(meraki_mac, "\n")] = 0;
    }
    fclose(macf);
  }

  if (dry_run) {
    printf("configd: dry-run mode enabled, no changes will be made\n");
  }

  // create config file if it doesn't exist
  if (access(config_file, F_OK) != 0) {
    if (dry_run) {
      printf("[dry-run] would create config file at %s\n", config_file);
    } else {
      printf("new config file created at %s\n", config_file);
      struct json_object *initial = click_read_ports();
      struct json_object *globals = click_read_globals();
      json_deep_merge(initial, globals);
      json_object_put(globals);
      save_config_file(initial);
      json_object_put(initial);
    }
  }

  struct lws_context *ctx = ws_init(ws_port);
  if (!ctx) {
    fprintf(stderr, "lws: context creation failed\n");
    return 1;
  }

  printf("configd: websocket server listening on port %d\n", ws_port);

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);

  ws_schedule_timers(ctx, status_interval);

  // main event loop
  while (running && lws_service(ctx, 0) >= 0) {
    mark_clients_pending();
  }

  printf("configd: shutting down\n");
  lws_context_destroy(ctx);
  free(cached_status_json);
  free(cached_config_json);

  return 0;
}

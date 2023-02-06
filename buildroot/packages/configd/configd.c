#include <libpostmerkos.h>
#include <libpd690xx.h>
#include "pd690xx_meraki.h"
#include "configd.h"

#include <dirent.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
// #include <sys/inotify.h>

bool poe_capable;
struct pd690xx_cfg pd690xx = {
    // i2c_fds
    {-1, -1},
    // pd690xx_addrs
    {PD690XX0_I2C_ADDR, PD690XX1_I2C_ADDR, PD690XX2_I2C_ADDR, PD690XX3_I2C_ADDR},
    // pd690xx_pres
    {0, 0, 0 ,0}
};

// read config from click filesystem
struct json_object *read_config() {

  struct json_object *jobj = json_object_new_object();

  struct json_object *jports = json_object_new_object();
  json_object_object_add(jobj, "ports", jports);


  FILE *pports = fopen(PORTS_FILE, "r");
  char pports_line[256];

  FILE *phy_cfgs = fopen("/click/switch_port_table/dump_port_phy_cfgs", "r");
  char phy_cfgs_line[256];

  int p = -1;
  char buffer[256];
  while (fgets(pports_line, sizeof(pports_line), pports)) {
    fgets(phy_cfgs_line, sizeof(phy_cfgs_line), phy_cfgs);

    p++;
    if (p == 0) {
      // skip file header
      continue;
    }

    struct json_object *jport = json_object_new_object();
    json_object_object_add(jports, itoa(p, buffer, 10), jport);

    bool enabled;
    if (strcmp(get_field(phy_cfgs_line, 2), "aneg") == 0) {
        enabled = true;
    }
    json_object_object_add(jport, "enabled",
                           json_object_new_boolean(enabled));

    if (poe_capable) {
      struct json_object *jportpoe = json_object_new_object();
      json_object_object_add(jport, "poe", jportpoe);
      json_object_object_add(jportpoe, "enabled",
                             json_object_new_boolean(port_state(&pd690xx, p)));
      json_object_object_add(jportpoe, "mode",
                             json_object_new_string(port_type_str(&pd690xx, p)));
    }
  }

  fclose(pports);
  fclose(phy_cfgs);
  return jobj;
}

// write config to click filesystem
int write_config(struct json_object *json) {

  // get all top-level keys
  json_object_object_foreach(json, key, value) {

    // for the "ports" key
    if (strcmp(key, "ports") == 0) {
      // get all ports
      json_object_object_foreach(value, port, portconfig) {
        // printf("%s  -> %s\n", port, json_object_get_string(config));

        // get config for a port
        json_object_object_foreach(portconfig, item, itemconfig) {
        //   printf("%s  -> %s\n", item, json_object_get_string(itemconfig));

          // for the "enabled" key
          if (strcmp(item, "enabled") == 0) {
            bool enabled = json_object_get_boolean(itemconfig);
            const char* state = get_field(read_switch_port_table("dump_port_phy_cfgs", atoi(port)), 2);

            if (enabled && (strcmp(state, "off") == 0)) {
              printf("port %s enabled", port);
              char command[40];
              snprintf(command, 40, "PORT %s, MODE aneg", port);
              write_switch_port_table("set_port_phy_cfgs", command);
            } else if (!enabled && (strcmp(state, "aneg") == 0)) {
              printf("port %s disabled", port);
              char command[40];
              snprintf(command, 40, "PORT %s, MODE off", port);
              write_switch_port_table("set_port_phy_cfgs", command);
            }
          }
          
          // for the "poe" key
          if (poe_capable && strcmp(item, "poe") == 0) {
            // get poe config
            json_object_object_foreach(itemconfig, poeitem, poeitemconfig) {

              // for the poe "enabled" key
              if (strcmp(poeitem, "enabled") == 0) {
                bool enabled = json_object_get_boolean(poeitemconfig);
                bool state = port_state(&pd690xx, atoi(port));

                if (enabled && (state == PORT_DISABLED)) {
                  port_enable(&pd690xx, atoi(port));
                } else if (!enabled && (state == PORT_ENABLED)) {
                  port_disable(&pd690xx, atoi(port));
                }
              }

            }
          }

        }

      }
    }

  }
}

// poll config file for changes
void poll() {
  time_t mtime;
  time(&mtime);
  struct stat st;
  struct json_object *current = read_config();
  struct json_object *modified;

  int file = open(CONFIG_FILE, O_RDONLY);
  if(file == -1) { 
    printf("unable to open file\n"); 
    return; 
  } 

  while(true) {
    // get file modification time
    if(fstat(file, &st))   { 
      printf("fstat error: [%s]\n", strerror(0)); 
      close(file); 
      return;
    }

    if (difftime(st.st_mtime, mtime) > 0) {
      mtime = st.st_mtime;
      modified = json_object_from_file(CONFIG_FILE);
      if (!json_object_equal(current, modified)) {
        current = modified;
        write_config(modified);
      }
    }

    sleep(1);
  }

}

int main(int argc, char **argv) {

  // determine whether board is poe capable
  poe_capable = has_poe();
  if (poe_capable) {
    i2c_init(&pd690xx);
  }

  // create config file if it doesn't exist
  if(access(CONFIG_FILE, F_OK) != 0) {
    printf("new config file created at %s\n", CONFIG_FILE);
    FILE *file = fopen(CONFIG_FILE, "w");
    const char * json = json_object_to_json_string_ext(
                   read_config(), JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
    fprintf(file, json);
    fclose(file);
  }

  // inotify is not currently enabled and returns the following error:
  //   inotify_init: Function not implemented
  // so lets poll for now
  poll();

}
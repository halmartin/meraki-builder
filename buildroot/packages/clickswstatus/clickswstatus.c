#include <libpostmerkos.h>
#include <libpd690xx.h>
#include "pd690xx_meraki.h"
#include "clickswstatus.h"

#include <dirent.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

bool poe_capable;
struct pd690xx_cfg pd690xx = {
    // i2c_fds
    {-1, -1},
    // pd690xx_addrs
    {PD690XX0_I2C_ADDR, PD690XX1_I2C_ADDR, PD690XX2_I2C_ADDR, PD690XX3_I2C_ADDR},
    // pd690xx_pres
    {0, 0, 0 ,0}
};

int main(int argc, char **argv) {
  i2c_init(&pd690xx);
  if (pd690xx_pres_count(&pd690xx)) {
    poe_capable = true;
  }

  struct json_object *jobj = json_object_new_object();
  json_object_object_add(jobj, "datetime", json_object_new_string(get_time()));

  struct json_object *jtemp = json_object_new_object();
  json_object_object_add(jobj, "temperature", jtemp);

  struct json_object *jtempsys = json_object_new_array();
  json_object_object_add(jtemp, "cpu", jtempsys);

  struct dirent *dp;
  DIR *dfd;
  char *dir = "/sys/class/thermal";
  if ((dfd = opendir(dir)) == NULL) {
    fprintf(stderr, "Can't open %s\n", dir);
    return 0;
  }

  char filename[100];
  while ((dp = readdir(dfd)) != NULL) {
    struct stat stbuf;
    sprintf(filename, "%s/%s", dir, dp->d_name);
    if (stat(filename, &stbuf) == -1) {
      printf("Unable to stat file: %s\n", filename);
      continue;
    }

    if (!starts_with(filename + strlen(dir) + 1, "thermal_")) {
      continue;
    }
    strcat(filename, "/temp");
    FILE *file = fopen(filename, "r");

    static char line[100];
    fgets(line, sizeof(line), file);
    // remove trailing newline
    line[strcspn(line, "\n")] = 0;
    json_object_array_add(jtempsys, json_object_new_int(atoi(line) / 1000));
    fclose(file);
  }

  if (poe_capable) {
    struct json_object *jtemppoe = json_object_new_array();
    json_object_object_add(jtemp, "poe", jtemppoe);
    float* temps = get_temp(&pd690xx);
    for (int i = 0; i < sizeof(temps); i++) {
    json_object_array_add(jtemppoe, json_object_new_double(temps[i]));
    }
  }

  struct json_object *jports = json_object_new_object();
  json_object_object_add(jobj, "ports", jports);

  FILE *file = fopen(PORTS_FILE, "r");
  char line[256];

  int p = -1;
  char buffer[256];
  while (fgets(line, sizeof(line), file)) {
    p++;
    if (p == 0) {
      // skip file header
      continue;
    }

    struct json_object *jport = json_object_new_object();
    json_object_object_add(jports, itoa(p, buffer, 10), jport);

    struct json_object *jportlink = json_object_new_object();
    json_object_object_add(jport, "link", jportlink);

    json_object_object_add(jportlink, "established",
                           json_object_new_boolean(atoi(get_field(line, 2))));
    json_object_object_add(jportlink, "speed",
                           json_object_new_int(atoi(get_field(line, 3))));

    if (poe_capable) {
      struct json_object *jportpoe = json_object_new_object();
      json_object_object_add(jport, "poe", jportpoe);
      json_object_object_add(jportpoe, "power",
                             json_object_new_double(port_power(&pd690xx, p)));
    }
  }

  fclose(file);
  printf("%s", json_object_to_json_string_ext(
                   jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
  json_object_put(jobj); // Delete the json object
}
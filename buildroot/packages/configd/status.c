#include "status.h"
#include "configd.h"

#include <libpostmerkos.h>
#include <libpd690xx.h>
#include "pd690xx_meraki.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// read device name from /etc/boardinfo
static const char *get_device_name(void) {
  static char name[64];
  FILE *f = fopen(DEVICE_FILE, "r");
  if (!f) return NULL;
  if (fgets(name, sizeof(name), f)) {
    name[strcspn(name, "\n")] = 0;
  }
  fclose(f);
  return name[0] ? name : NULL;
}

static void add_device_name(struct json_object *jobj) {
  const char *device = get_device_name();
  if (device) {
    json_object_object_add(jobj, "device", json_object_new_string(device));
  }
}

static void add_temperatures(struct json_object *jobj) {
  struct json_object *jtemp = json_object_new_object();
  json_object_object_add(jobj, "temperature", jtemp);

  struct json_object *jtempsys = json_object_new_array();
  json_object_object_add(jtemp, "cpu", jtempsys);

  struct dirent *dp;
  DIR *dfd;
  char *dir = "/sys/class/thermal";
  if ((dfd = opendir(dir)) != NULL) {
    char filename[100];
    while ((dp = readdir(dfd)) != NULL) {
      struct stat stbuf;
      sprintf(filename, "%s/%s", dir, dp->d_name);
      if (stat(filename, &stbuf) == -1)
        continue;
      if (!starts_with(filename + strlen(dir) + 1, "thermal_"))
        continue;
      strcat(filename, "/temp");
      FILE *file = fopen(filename, "r");
      if (!file) continue;
      static char line[100];
      fgets(line, sizeof(line), file);
      line[strcspn(line, "\n")] = 0;
      json_object_array_add(jtempsys, json_object_new_int(atoi(line) / 1000));
      fclose(file);
    }
    closedir(dfd);
  }

  if (poe_capable) {
    struct json_object *jtemppoe = json_object_new_array();
    json_object_object_add(jtemp, "poe", jtemppoe);
    float *temps = get_temp(&pd690xx);
    for (int i = 0; i < pd690xx_pres_count(&pd690xx); i++) {
      json_object_array_add(jtemppoe, json_object_new_double(temps[i]));
    }
    free(temps);
  }
}

static void add_port_status(struct json_object *jobj) {
  struct json_object *jports = json_object_new_object();
  json_object_object_add(jobj, "ports", jports);

  FILE *file = fopen(PORTS_FILE, "r");
  if (!file) return;

  char line[256];
  int p = -1;
  char buffer[256];
  while (fgets(line, sizeof(line), file)) {
    p++;
    if (p == 0) continue; // skip header

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
}

struct json_object *get_status(void) {
  struct json_object *jobj = json_object_new_object();
  json_object_object_add(jobj, "datetime", json_object_new_string(get_time()));
  add_device_name(jobj);
  add_temperatures(jobj);
  add_port_status(jobj);
  return jobj;
}

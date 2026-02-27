#include "click_global.h"
#include "configd.h"

#include <libpostmerkos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- STP global ---

static struct json_object *read_stp_global(void) {
  struct json_object *stp = json_object_new_object();
  json_object_object_add(stp, "priority", json_object_new_int(32768));
  json_object_object_add(stp, "hello_time", json_object_new_int(2));
  json_object_object_add(stp, "forward_delay", json_object_new_int(15));
  json_object_object_add(stp, "max_age", json_object_new_int(20));
  json_object_object_add(stp, "hold_count", json_object_new_int(6));
  return stp;
}

static int apply_stp_global(struct json_object *value) {
  struct json_object *obj;

  int priority = 32768;
  if (json_object_object_get_ex(value, "priority", &obj))
    priority = json_object_get_int(obj);

  int hello_time = 2;
  if (json_object_object_get_ex(value, "hello_time", &obj))
    hello_time = json_object_get_int(obj);

  int forward_delay = 15;
  if (json_object_object_get_ex(value, "forward_delay", &obj))
    forward_delay = json_object_get_int(obj);

  int max_age = 20;
  if (json_object_object_get_ex(value, "max_age", &obj))
    max_age = json_object_get_int(obj);

  int hold_count = 6;
  if (json_object_object_get_ex(value, "hold_count", &obj))
    hold_count = json_object_get_int(obj);

  char command[256];
  snprintf(command, sizeof(command),
           "PRIORITY %d, HELLO_TIME %d, FORWARD_DELAY %d, MAX_AGE %d, HOLDCOUNT %d",
           priority, hello_time, forward_delay, max_age, hold_count);

  printf("%s global field=stp value=%s%s\n",
         get_time(), command, dry_run ? " dry_run=true" : "");
  if (!dry_run) {
    click_write("/click/stp/set_params", command);
  }
  return 0;
}

// --- LACP ---

static struct json_object *read_lacp(void) {
  struct json_object *lacp = json_object_new_object();
  char buf[16];
  if (click_read("/click/switch_port_table/enable_lacp_on_single_ports",
                 buf, sizeof(buf))) {
    buf[strcspn(buf, "\n")] = 0;
    json_object_object_add(lacp, "enabled",
                           json_object_new_boolean(strcmp(buf, "true") == 0));
  } else {
    json_object_object_add(lacp, "enabled", json_object_new_boolean(true));
  }
  return lacp;
}

static int apply_lacp(struct json_object *value) {
  struct json_object *obj;
  if (!json_object_object_get_ex(value, "enabled", &obj)) return 0;
  bool enabled = json_object_get_boolean(obj);

  printf("%s global field=lacp enabled=%s%s\n",
         get_time(), enabled ? "true" : "false",
         dry_run ? " dry_run=true" : "");
  if (!dry_run) {
    write_switch_port_table("enable_lacp_on_single_ports",
                            enabled ? "true" : "false");
  }
  return 0;
}

// --- multicast (IGMP/MLD snooping) ---

static struct json_object *read_multicast(void) {
  struct json_object *mc = json_object_new_object();
  json_object_object_add(mc, "igmp_snooping", json_object_new_boolean(true));
  json_object_object_add(mc, "igmp_querier_interval", json_object_new_int(125));
  json_object_object_add(mc, "mld_snooping", json_object_new_boolean(true));
  json_object_object_add(mc, "mld_querier_interval", json_object_new_int(125));
  return mc;
}

static int apply_multicast(struct json_object *value) {
  struct json_object *obj;

  bool igmp_snoop = true;
  if (json_object_object_get_ex(value, "igmp_snooping", &obj))
    igmp_snoop = json_object_get_boolean(obj);

  int igmp_interval = 125;
  if (json_object_object_get_ex(value, "igmp_querier_interval", &obj))
    igmp_interval = json_object_get_int(obj);

  bool mld_snoop = true;
  if (json_object_object_get_ex(value, "mld_snooping", &obj))
    mld_snoop = json_object_get_boolean(obj);

  int mld_interval = 125;
  if (json_object_object_get_ex(value, "mld_querier_interval", &obj))
    mld_interval = json_object_get_int(obj);

  printf("%s global field=multicast igmp=%s/%d mld=%s/%d%s\n",
         get_time(),
         igmp_snoop ? "true" : "false", igmp_interval,
         mld_snoop ? "true" : "false", mld_interval,
         dry_run ? " dry_run=true" : "");

  if (!dry_run) {
    click_write("/click/configure_igmp_snoop/run",
                igmp_snoop ? "true" : "false");

    char buf[32];
    // querier interval in milliseconds
    snprintf(buf, sizeof(buf), "%d", igmp_interval * 1000);
    click_write("/click/igmp_snoop/default_querier_interval_msec", buf);

    snprintf(buf, sizeof(buf), "%d", igmp_interval);
    click_write("/click/igmp_querier/query_interval", buf);

    click_write("/click/configure_mld_snoop/run",
                mld_snoop ? "true" : "false");

    snprintf(buf, sizeof(buf), "%d", mld_interval * 1000);
    click_write("/click/mld_snoop/default_querier_interval_msec", buf);

    snprintf(buf, sizeof(buf), "%d", mld_interval);
    click_write("/click/mld_querier/query_interval", buf);
  }
  return 0;
}

// --- global field table ---

const struct global_field global_fields[] = {
  { "stp",       read_stp_global,  apply_stp_global },
  { "lacp",      read_lacp,        apply_lacp       },
  { "multicast", read_multicast,   apply_multicast  },
};

const int global_field_count =
    sizeof(global_fields) / sizeof(global_fields[0]);

// --- generic read/apply loops ---

struct json_object *click_read_globals(void) {
  struct json_object *jobj = json_object_new_object();
  for (int i = 0; i < global_field_count; i++) {
    if (!global_fields[i].read) continue;
    json_object_object_add(jobj, global_fields[i].key,
                           global_fields[i].read());
  }
  return jobj;
}

int click_apply_globals(struct json_object *config) {
  for (int i = 0; i < global_field_count; i++) {
    if (!global_fields[i].apply) continue;
    struct json_object *val;
    if (!json_object_object_get_ex(config, global_fields[i].key, &val))
      continue;
    global_fields[i].apply(val);
  }
  return 0;
}

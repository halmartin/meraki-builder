#include "click_port.h"
#include "configd.h"

#include <libpostmerkos.h>
#include <libpd690xx.h>
#include "pd690xx_meraki.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// print a structured change log entry to stdout
static void log_change(int port, const char *field,
                       const char *old_val, const char *new_val) {
  printf("%s port=%d field=%s old=%s new=%s%s\n",
         get_time(), port, field, old_val, new_val,
         dry_run ? " dry_run=true" : "");
}

// --- speed mapping ---

struct speed_map {
  const char *ui;    // JSON value: "auto", "10half", etc.
  const char *click; // Click MODE: "aneg", "10hdx", etc.
};

static const struct speed_map speed_table[] = {
  { "auto",     "aneg"    },
  { "10half",   "10hdx"   },
  { "10full",   "10fdx"   },
  { "100half",  "100hdx"  },
  { "100full",  "100fdx"  },
  { "1000full", "1000fdx" },
};

static const int speed_table_count =
    sizeof(speed_table) / sizeof(speed_table[0]);

static const char *speed_click_to_ui(const char *click_mode) {
  for (int i = 0; i < speed_table_count; i++) {
    if (strcmp(speed_table[i].click, click_mode) == 0)
      return speed_table[i].ui;
  }
  return "auto";
}

static const char *speed_ui_to_click(const char *ui_speed) {
  for (int i = 0; i < speed_table_count; i++) {
    if (strcmp(speed_table[i].ui, ui_speed) == 0)
      return speed_table[i].click;
  }
  return "aneg";
}

// --- enabled field ---

static struct json_object *read_port_enabled(int port) {
  char *line = read_switch_port_table("dump_port_phy_cfgs", port);
  const char *mode = get_field(line, 2);
  bool enabled = mode && strcmp(mode, "off") != 0;
  free(line);
  return json_object_new_boolean(enabled);
}

// --- speed field ---

static struct json_object *read_port_speed(int port) {
  char *line = read_switch_port_table("dump_port_phy_cfgs", port);
  const char *mode = get_field(line, 2);
  const char *ui = "auto";
  if (mode && strcmp(mode, "off") != 0) {
    ui = speed_click_to_ui(mode);
  }
  free(line);
  return json_object_new_string(ui);
}

// --- flow_control field ---

static struct json_object *read_port_fc(int port) {
  (void)port;
  // default: flow control off (FC_OBEY field not reliably readable from dump)
  return json_object_new_boolean(false);
}

// --- eee field ---

static struct json_object *read_port_eee(int port) {
  (void)port;
  // default: EEE enabled
  return json_object_new_boolean(true);
}

// --- apply_phy: compound apply for enabled + speed + flow_control + eee ---

static int apply_phy(int port, struct json_object *port_config) {
  struct json_object *obj;

  // read enabled (required)
  bool enabled = true;
  if (json_object_object_get_ex(port_config, "enabled", &obj))
    enabled = json_object_get_boolean(obj);

  // read speed
  const char *speed = "auto";
  if (json_object_object_get_ex(port_config, "speed", &obj))
    speed = json_object_get_string(obj);

  // read flow_control
  bool fc = false;
  if (json_object_object_get_ex(port_config, "flow_control", &obj))
    fc = json_object_get_boolean(obj);

  // read eee
  bool eee = true;
  if (json_object_object_get_ex(port_config, "eee", &obj))
    eee = json_object_get_boolean(obj);

  const char *mode = enabled ? speed_ui_to_click(speed) : "off";

  char command[128];
  snprintf(command, sizeof(command),
           "PORT %d, FC_OBEY %s, EEE_ADV_ENABLED %s, MODE %s",
           port, fc ? "true" : "false", eee ? "true" : "false", mode);

  log_change(port, "phy", "", command);
  if (!dry_run) {
    write_switch_port_table("set_port_phy_cfgs", command);
  }
  return 0;
}

// --- storm_control field ---

static struct json_object *read_storm_control(int port) {
  char *line = read_switch_port_table("dump_port_storm_control", port);
  if (!line) return json_object_new_boolean(true);
  // field 2 is ENABLED (true/false)
  const char *val = get_field(line, 2);
  bool enabled = !val || strcmp(val, "true") == 0;
  free(line);
  return json_object_new_boolean(enabled);
}

static int apply_storm_control(int port, struct json_object *port_config) {
  struct json_object *obj;
  if (!json_object_object_get_ex(port_config, "storm_control", &obj)) return 0;
  bool enabled = json_object_get_boolean(obj);

  char command[64];
  snprintf(command, sizeof(command), "PORT %d, ENABLED %s",
           port, enabled ? "true" : "false");

  log_change(port, "storm_control", "", enabled ? "true" : "false");
  if (!dry_run) {
    write_switch_port_table("set_port_storm_control", command);
  }
  return 0;
}

// --- vlan field ---

static struct json_object *read_port_vlan(int port) {
  struct json_object *vlan = json_object_new_object();
  char *line = read_switch_port_table("dump_pport_vlans", port);
  if (!line) {
    // defaults
    json_object_object_add(vlan, "mode", json_object_new_string("access"));
    json_object_object_add(vlan, "pvid", json_object_new_int(1));
    json_object_object_add(vlan, "allowed", json_object_new_string(""));
    json_object_object_add(vlan, "untagged_vid", json_object_new_int(1));
    json_object_object_add(vlan, "ingress_filter", json_object_new_boolean(true));
    return vlan;
  }

  // dump_pport_vlans fields: 1=port, 2=tag_in, 3=untag_in, ... 6=pvid, 7=untagged_vid, ... 11=allowed
  const char *tag_in_str = get_field(line, 2);
  const char *untag_in_str = get_field(line, 3);
  const char *pvid_str = get_field(line, 6);
  const char *untag_vid_str = get_field(line, 7);
  const char *allowed_str = get_field(line, 11);

  bool tag_in = tag_in_str && strcmp(tag_in_str, "1") == 0;
  bool untag_in = !untag_in_str || strcmp(untag_in_str, "1") == 0;

  // derive mode
  const char *mode = "access";
  if (tag_in) mode = "trunk";

  json_object_object_add(vlan, "mode", json_object_new_string(mode));
  json_object_object_add(vlan, "pvid",
                         json_object_new_int(pvid_str ? atoi(pvid_str) : 1));
  json_object_object_add(vlan, "allowed",
                         json_object_new_string(allowed_str ? allowed_str : ""));
  json_object_object_add(vlan, "untagged_vid",
                         json_object_new_int(untag_vid_str ? atoi(untag_vid_str) : 1));
  json_object_object_add(vlan, "ingress_filter", json_object_new_boolean(true));

  (void)untag_in;
  free(line);
  return vlan;
}

static int apply_port_vlan(int port, struct json_object *port_config) {
  struct json_object *vlan;
  if (!json_object_object_get_ex(port_config, "vlan", &vlan)) return 0;

  struct json_object *obj;
  const char *mode = "access";
  if (json_object_object_get_ex(vlan, "mode", &obj))
    mode = json_object_get_string(obj);

  int pvid = 1;
  if (json_object_object_get_ex(vlan, "pvid", &obj))
    pvid = json_object_get_int(obj);

  const char *allowed = "";
  if (json_object_object_get_ex(vlan, "allowed", &obj))
    allowed = json_object_get_string(obj);

  int untagged_vid = 1;
  if (json_object_object_get_ex(vlan, "untagged_vid", &obj))
    untagged_vid = json_object_get_int(obj);

  bool ingress_filter = true;
  if (json_object_object_get_ex(vlan, "ingress_filter", &obj))
    ingress_filter = json_object_get_boolean(obj);

  // derive click booleans from mode
  bool tag_in = strcmp(mode, "access") != 0;   // trunk/hybrid allow tagged
  bool untag_in = true;                        // all modes allow untagged

  // access mode: allowed = pvid only
  char allowed_buf[32];
  if (strcmp(mode, "access") == 0) {
    snprintf(allowed_buf, sizeof(allowed_buf), "%d", pvid);
    allowed = allowed_buf;
  }

  char command[256];
  snprintf(command, sizeof(command),
           "PORT %d, ALLOWED_VLANS %s, ALLOW_TAGGED_IN %s, ALLOW_UNTAGGED_IN %s, "
           "INGRESS_FILTER %s, RADIUS_CAN_ASSIGN_VLAN false, PVID %d, UNTAGGED_VID %d",
           port, allowed, tag_in ? "true" : "false", untag_in ? "true" : "false",
           ingress_filter ? "true" : "false", pvid, untagged_vid);

  log_change(port, "vlan", "", command);
  if (!dry_run) {
    write_switch_port_table("set_vlan_allports_conf", command);
  }
  return 0;
}

// --- stp per-port field ---

static struct json_object *read_port_stp(int port) {
  (void)port;
  struct json_object *stp = json_object_new_object();
  // S10clickconfig defaults
  json_object_object_add(stp, "enabled", json_object_new_boolean(true));
  json_object_object_add(stp, "priority", json_object_new_int(128));
  json_object_object_add(stp, "cost", json_object_new_int(0));
  json_object_object_add(stp, "edge", json_object_new_boolean(false));
  json_object_object_add(stp, "auto_edge", json_object_new_boolean(true));
  return stp;
}

static int apply_port_stp(int port, struct json_object *port_config) {
  struct json_object *stp;
  if (!json_object_object_get_ex(port_config, "stp", &stp)) return 0;

  if (meraki_mac[0] == '\0') {
    fprintf(stderr, "warning: meraki_mac not set, skipping STP port apply\n");
    return -1;
  }

  struct json_object *obj;

  bool enabled = true;
  if (json_object_object_get_ex(stp, "enabled", &obj))
    enabled = json_object_get_boolean(obj);

  int priority = 128;
  if (json_object_object_get_ex(stp, "priority", &obj))
    priority = json_object_get_int(obj);

  int cost = 0;
  if (json_object_object_get_ex(stp, "cost", &obj))
    cost = json_object_get_int(obj);

  bool edge = false;
  if (json_object_object_get_ex(stp, "edge", &obj))
    edge = json_object_get_boolean(obj);

  bool auto_edge = true;
  if (json_object_object_get_ex(stp, "auto_edge", &obj))
    auto_edge = json_object_get_boolean(obj);

  char command[256];
  snprintf(command, sizeof(command),
           "PORT %s/%d, ENABLED %s, AUTOEDGE %s, EDGE %s, "
           "AUTOPTP true, PTP false, PRI %d, COST %d",
           meraki_mac, port,
           enabled ? "true" : "false",
           auto_edge ? "true" : "false",
           edge ? "true" : "false",
           priority, cost);

  log_change(port, "stp", "", command);
  if (!dry_run) {
    click_write("/click/stp/set_many_port_cfgs", command);
  }
  return 0;
}

// --- poe field ---

static struct json_object *read_port_poe(int port) {
  struct json_object *poe = json_object_new_object();
  json_object_object_add(poe, "enabled",
                         json_object_new_boolean(port_state(&pd690xx, port)));
  json_object_object_add(poe, "mode",
                         json_object_new_string(port_type_str(&pd690xx, port)));
  return poe;
}

static int apply_port_poe(int port, struct json_object *port_config) {
  struct json_object *value;
  if (!json_object_object_get_ex(port_config, "poe", &value)) return 0;
  struct json_object *enabled_obj;
  if (json_object_object_get_ex(value, "enabled", &enabled_obj)) {
    bool enabled = json_object_get_boolean(enabled_obj);
    bool state = port_state(&pd690xx, port);

    if (enabled && state == PORT_DISABLED) {
      log_change(port, "poe.enabled", "false", "true");
      if (!dry_run) {
        port_enable(&pd690xx, port);
      }
    } else if (!enabled && state == PORT_ENABLED) {
      log_change(port, "poe.enabled", "true", "false");
      if (!dry_run) {
        port_disable(&pd690xx, port);
      }
    }
  }
  return 0;
}

// --- field table ---

const struct port_field port_fields[] = {
  { "enabled",       read_port_enabled,   apply_phy,           false },
  { "speed",         read_port_speed,     NULL,                false },
  { "flow_control",  read_port_fc,        NULL,                false },
  { "eee",           read_port_eee,       NULL,                false },
  { "storm_control", read_storm_control,  apply_storm_control, false },
  { "vlan",          read_port_vlan,      apply_port_vlan,     false },
  { "stp",           read_port_stp,       apply_port_stp,      false },
  { "poe",           read_port_poe,       apply_port_poe,      true  },
};

const int port_field_count = sizeof(port_fields) / sizeof(port_fields[0]);

// --- generic read/apply loops ---

struct json_object *click_read_ports(void) {
  struct json_object *jobj = json_object_new_object();
  struct json_object *jports = json_object_new_object();
  json_object_object_add(jobj, "ports", jports);

  FILE *pports = fopen(PORTS_FILE, "r");
  if (!pports) return jobj;

  char line[256];
  char buffer[256];
  int p = -1;
  while (fgets(line, sizeof(line), pports)) {
    p++;
    if (p == 0) continue; // skip header

    struct json_object *jport = json_object_new_object();
    json_object_object_add(jports, itoa(p, buffer, 10), jport);

    for (int i = 0; i < port_field_count; i++) {
      if (port_fields[i].requires_poe && !poe_capable) continue;
      if (!port_fields[i].read) continue;
      json_object_object_add(jport, port_fields[i].key,
                             port_fields[i].read(p));
    }
  }

  fclose(pports);
  return jobj;
}

int click_apply_ports(struct json_object *config) {
  struct json_object *ports;
  if (!json_object_object_get_ex(config, "ports", &ports)) return 0;

  json_object_object_foreach(ports, port_str, port_config) {
    int port = atoi(port_str);
    for (int i = 0; i < port_field_count; i++) {
      if (port_fields[i].requires_poe && !poe_capable) continue;
      if (!port_fields[i].apply) continue;
      struct json_object *val;
      if (!json_object_object_get_ex(port_config, port_fields[i].key, &val))
        continue;
      port_fields[i].apply(port, port_config);
    }
  }
  return 0;
}

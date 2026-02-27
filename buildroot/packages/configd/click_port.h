#ifndef CLICK_PORT_H
#define CLICK_PORT_H

#include <json-c/json.h>
#include <stdbool.h>

typedef struct json_object *(*field_read_fn)(int port);
typedef int (*field_apply_fn)(int port, struct json_object *port_config);

struct port_field {
  const char *key;
  field_read_fn read;
  field_apply_fn apply;
  bool requires_poe;
};

extern const struct port_field port_fields[];
extern const int port_field_count;

// read current port config from /click → JSON
struct json_object *click_read_ports(void);

// apply JSON config → /click
int click_apply_ports(struct json_object *config);

#endif

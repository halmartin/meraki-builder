#ifndef CLICK_GLOBAL_H
#define CLICK_GLOBAL_H

#include <json-c/json.h>

typedef struct json_object *(*global_read_fn)(void);
typedef int (*global_apply_fn)(struct json_object *value);

struct global_field {
  const char *key;
  global_read_fn read;
  global_apply_fn apply;
};

extern const struct global_field global_fields[];
extern const int global_field_count;

// read current global config from /click -> JSON
struct json_object *click_read_globals(void);

// apply global settings (stp, lacp, multicast) from config JSON to /click
int click_apply_globals(struct json_object *config);

#endif

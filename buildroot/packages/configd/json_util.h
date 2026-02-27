#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <json-c/json.h>

// recursively merge patch into base (modifies base in place)
void json_deep_merge(struct json_object *base, struct json_object *patch);

#endif

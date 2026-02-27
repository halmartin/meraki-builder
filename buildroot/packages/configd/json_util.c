#include "json_util.h"

// recursively merge patch into base (modifies base in place)
void json_deep_merge(struct json_object *base, struct json_object *patch) {
  json_object_object_foreach(patch, key, patch_val) {
    struct json_object *base_val;
    if (json_object_object_get_ex(base, key, &base_val) &&
        json_object_is_type(base_val, json_type_object) &&
        json_object_is_type(patch_val, json_type_object)) {
      json_deep_merge(base_val, patch_val);
    } else {
      json_object_object_add(base, key, json_object_get(patch_val));
    }
  }
}

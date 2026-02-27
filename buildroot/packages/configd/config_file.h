#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <json-c/json.h>

// load config JSON from disk (returns NULL on failure)
struct json_object *load_config_file(void);

// save config JSON to disk (returns 0 on success, -1 on failure)
int save_config_file(struct json_object *json);

#endif

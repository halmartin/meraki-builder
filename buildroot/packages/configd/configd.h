#ifndef configd
#define configd

#define CONFIG_FILE "/etc/switch.json"

struct json_object * read_config();
int write_config(struct json_object *json);

void poll();

#endif
#ifndef configd
#define configd

struct json_object * read_config();
int write_config(struct json_object *json);

void poll();

#endif
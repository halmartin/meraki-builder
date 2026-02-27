#include "config_file.h"
#include "configd.h"

#include <libpostmerkos.h>
#include <stdio.h>

struct json_object *load_config_file(void) {
  return json_object_from_file(config_file);
}

int save_config_file(struct json_object *json) {
  if (dry_run) return 0;

  const char *json_str = json_object_to_json_string_ext(
      json, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
  FILE *f = fopen(config_file, "w");
  if (!f) {
    fprintf(stderr, "%s config: failed to write %s\n",
            get_time(), config_file);
    return -1;
  }
  fprintf(f, "%s", json_str);
  fclose(f);
  printf("%s config: written to %s\n", get_time(), config_file);
  return 0;
}

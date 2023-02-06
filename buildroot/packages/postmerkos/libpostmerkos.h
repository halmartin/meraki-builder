#ifndef postmerkos
#define postmerkos

#define DEVICE_FILE "/etc/boardinfo"
#define PORTS_FILE "/click/switch_port_table/dump_pports"

_Bool has_poe();
char *get_time();
_Bool starts_with(const char*, const char*);
_Bool ends_with(const char*, const char *);
char *itoa(int, char*, int);
const char *get_field(char*, int);

void write_switch_port_table(char* filename, char* str);
char* read_switch_port_table(char* filename, int port);

#endif // postmerkos
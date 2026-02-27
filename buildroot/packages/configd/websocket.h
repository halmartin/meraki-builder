#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <libwebsockets.h>
#include <json-c/json.h>

#define MAX_CLIENTS 8
#define MAX_MSG_LEN 65536

// initialize websocket server, returns context or NULL on failure
struct lws_context *ws_init(int port);

// schedule status and config polling timers
void ws_schedule_timers(struct lws_context *ctx, int status_interval);

// wrap a JSON object in a typed message envelope
char *wrap_message(const char *type, struct json_object *data);

// mark all clients as having pending data
void mark_clients_pending(void);

// cached messages for broadcast
extern char *cached_status_json;
extern char *cached_config_json;

#endif

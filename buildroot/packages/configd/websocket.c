#include "websocket.h"
#include "configd.h"
#include "status.h"
#include "click_port.h"
#include "click_global.h"
#include "json_util.h"
#include "config_file.h"

#include <libpostmerkos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// connected client tracking
static struct lws *clients[MAX_CLIENTS];
static int client_count = 0;

// pending broadcast flags
static bool status_pending = false;
static bool config_pending = false;

// cached JSON strings for broadcast
char *cached_status_json = NULL;
char *cached_config_json = NULL;

// config file mtime tracking
static time_t config_mtime;

// scheduled timers
static struct lws_sorted_usec_list status_sul;
static struct lws_sorted_usec_list config_sul;

// websocket context (set during init)
static struct lws_context *ws_context;

static void add_client(struct lws *wsi) {
  if (client_count < MAX_CLIENTS) {
    clients[client_count++] = wsi;
  }
}

static void remove_client(struct lws *wsi) {
  for (int i = 0; i < client_count; i++) {
    if (clients[i] == wsi) {
      clients[i] = clients[--client_count];
      return;
    }
  }
}

static void request_writable_all(void) {
  for (int i = 0; i < client_count; i++) {
    lws_callback_on_writable(clients[i]);
  }
}

// wrap a JSON object in a typed message envelope: {"type":"...","data":...}
char *wrap_message(const char *type, struct json_object *data) {
  struct json_object *msg = json_object_new_object();
  json_object_object_add(msg, "type", json_object_new_string(type));
  json_object_object_add(msg, "data", json_object_get(data));
  const char *str = json_object_to_json_string_ext(msg, JSON_C_TO_STRING_PLAIN);
  char *result = strdup(str);
  json_object_put(msg);
  return result;
}

// send a JSON string over WebSocket with LWS_PRE padding
static int ws_send(struct lws *wsi, const char *json_str) {
  size_t len = strlen(json_str);
  unsigned char *buf = malloc(LWS_PRE + len);
  if (!buf) return -1;
  memcpy(buf + LWS_PRE, json_str, len);
  int written = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
  free(buf);
  return written;
}

// status polling callback (lws_sul)
static void status_poll_cb(struct lws_sorted_usec_list *sul) {
  struct json_object *status = get_status();
  char *msg = wrap_message("status", status);

  // compare with cached; only broadcast if changed
  if (!cached_status_json || strcmp(cached_status_json, msg) != 0) {
    free(cached_status_json);
    cached_status_json = msg;
    status_pending = true;
    request_writable_all();
  } else {
    free(msg);
  }

  json_object_put(status);

  // reschedule
  lws_sul_schedule(ws_context, 0, sul, status_poll_cb,
                   3 * LWS_USEC_PER_SEC);
}

// config file polling callback (lws_sul)
static void config_poll_cb(struct lws_sorted_usec_list *sul) {
  struct stat st;
  if (stat(config_file, &st) == 0 && difftime(st.st_mtime, config_mtime) > 0) {
    config_mtime = st.st_mtime;
    struct json_object *modified = load_config_file();
    if (modified) {
      click_apply_ports(modified);
      click_apply_globals(modified);

      char *msg = wrap_message("config", modified);
      free(cached_config_json);
      cached_config_json = msg;
      config_pending = true;
      request_writable_all();

      json_object_put(modified);
    }
  }

  // reschedule
  lws_sul_schedule(ws_context, 0, sul, config_poll_cb,
                   10 * LWS_USEC_PER_SEC);
}

// per-session data for tracking what this client needs
struct per_session_data {
  bool send_initial_status;
  bool send_initial_config;
  bool send_status;
  bool send_config;
};

static int
configd_ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                    void *user, void *in, size_t len) {
  struct per_session_data *pss = (struct per_session_data *)user;

  switch (reason) {
  case LWS_CALLBACK_ESTABLISHED:
    add_client(wsi);
    pss->send_initial_status = true;
    pss->send_initial_config = true;
    pss->send_status = false;
    pss->send_config = false;
    lws_callback_on_writable(wsi);
    printf("ws: client connected (%d total)\n", client_count);
    break;

  case LWS_CALLBACK_CLOSED:
    remove_client(wsi);
    printf("ws: client disconnected (%d total)\n", client_count);
    break;

  case LWS_CALLBACK_RECEIVE: {
    // parse incoming message
    struct json_object *msg = json_tokener_parse((const char *)in);
    if (!msg) break;

    struct json_object *type_obj;
    if (!json_object_object_get_ex(msg, "type", &type_obj)) {
      json_object_put(msg);
      break;
    }

    const char *type = json_object_get_string(type_obj);
    if (strcmp(type, "config") == 0) {
      struct json_object *data_obj;
      if (json_object_object_get_ex(msg, "data", &data_obj)) {
        printf("%s config: received delta: %s\n", get_time(),
               json_object_to_json_string(data_obj));

        // read current config and merge delta into it
        struct json_object *current = load_config_file();
        if (!current) current = json_object_new_object();
        json_deep_merge(current, data_obj);

        // write merged config to disk
        save_config_file(current);

        // apply merged config to /click
        click_apply_ports(current);
        click_apply_globals(current);

        // update mtime tracking
        struct stat st;
        if (stat(config_file, &st) == 0) {
          config_mtime = st.st_mtime;
        }

        // broadcast full merged config to all clients
        char *broadcast = wrap_message("config", current);
        free(cached_config_json);
        cached_config_json = broadcast;
        config_pending = true;

        // immediately refresh status so clients see the effect of config changes
        struct json_object *new_status = get_status();
        char *status_msg = wrap_message("status", new_status);
        free(cached_status_json);
        cached_status_json = status_msg;
        status_pending = true;
        json_object_put(new_status);

        request_writable_all();

        json_object_put(current);
      }
    }

    json_object_put(msg);
    break;
  }

  case LWS_CALLBACK_SERVER_WRITEABLE:
    // send initial status on connect
    if (pss->send_initial_status && cached_status_json) {
      ws_send(wsi, cached_status_json);
      pss->send_initial_status = false;
      if (pss->send_initial_config || pss->send_status || pss->send_config)
        lws_callback_on_writable(wsi);
      break;
    }
    // send initial config on connect
    if (pss->send_initial_config) {
      struct json_object *cfg = load_config_file();
      if (cfg) {
        char *msg = wrap_message("config", cfg);
        ws_send(wsi, msg);
        free(msg);
        json_object_put(cfg);
      }
      pss->send_initial_config = false;
      if (pss->send_status || pss->send_config)
        lws_callback_on_writable(wsi);
      break;
    }
    // broadcast status update
    if (pss->send_status && cached_status_json) {
      ws_send(wsi, cached_status_json);
      pss->send_status = false;
      if (pss->send_config)
        lws_callback_on_writable(wsi);
      break;
    }
    // broadcast config update
    if (pss->send_config && cached_config_json) {
      ws_send(wsi, cached_config_json);
      pss->send_config = false;
      break;
    }
    break;

  default:
    break;
  }

  return 0;
}

// mark all clients as having pending data when broadcasts are flagged
void mark_clients_pending(void) {
  if (!status_pending && !config_pending)
    return;

  for (int i = 0; i < client_count; i++) {
    struct per_session_data *pss =
        (struct per_session_data *)lws_wsi_user(clients[i]);
    if (pss) {
      if (status_pending) pss->send_status = true;
      if (config_pending) pss->send_config = true;
    }
  }

  status_pending = false;
  config_pending = false;
}

static const struct lws_protocols protocols[] = {
    {
        "configd-ws",
        configd_ws_callback,
        sizeof(struct per_session_data),
        MAX_MSG_LEN,
    },
    LWS_PROTOCOL_LIST_TERM
};

struct lws_context *ws_init(int port) {
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.port = port;
  info.protocols = protocols;
  info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE
               | LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;

  lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

  ws_context = lws_create_context(&info);
  return ws_context;
}

void ws_schedule_timers(struct lws_context *ctx, int status_interval) {
  // initialize config mtime
  struct stat st;
  if (stat(config_file, &st) == 0) {
    config_mtime = st.st_mtime;
  }

  // seed initial status cache
  struct json_object *initial_status = get_status();
  cached_status_json = wrap_message("status", initial_status);
  json_object_put(initial_status);

  // schedule periodic timers
  lws_sul_schedule(ctx, 0, &status_sul, status_poll_cb,
                   status_interval * LWS_USEC_PER_SEC);
  lws_sul_schedule(ctx, 0, &config_sul, config_poll_cb,
                   10 * LWS_USEC_PER_SEC);
}

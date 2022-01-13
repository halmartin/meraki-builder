# switch configuration

This doc describes how a postmerkos switch is configured.

> **NOTE**: If you are an end-user looking to manage your swich, we recommend the [UI](#UI).
> The rest of this doc largely goes into detail about how all of the parts are stitched together under the hood.

## click

The underlying interface is a filesystem mounted at `/click`.

> **TODO**: are there any docs we can reference?

If you're developing code to run directly on the switch, you may need to go through `click`.
All I can say is: search the repo for some examples.

Most common settings can be modified through one of the more user-friendly options described below.

## json

The JSON file at `/etc/switch.json` is intended to be the source of truth for user-level configuration.

It should define everything most users will need (if not, please open an issue).
You _can_ edit this file directly but there are other options, described in the sections below, which provide features such as type-checking and validation.


### schema

The [JSON Schema](https://json-schema.org/) for this file is defined below.

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "postmerkos/switch.schema.json",
  "title": "switch",
  "description": "network switch configuration for the postmerkos firmware",
  "type": "object",
  "required": [ "ports" ],
  "properties": {
    "ports": {
      "description": "configuration for an individual port",
      "type": "object",
      "patternProperties": {
        "^[0-9]*$": {
          "type": "object",
          "properties": {
            "enabled": {
              "description": "whether or not the port is enabled",
              "type": "boolean"
            },
            "poe": {
              "description": "settings related to power-over-ethernet",
              "type": "object",
              "properties": {
                "enabled": {
                  "description": "whether PoE is enabled",
                  "type": "boolean"
                },
                "mode": {
                  "description": "PoE mode",
                  "enum": ["802.3af", "802.3at"]
                }
              }
            }
          }
        }
      }
    }
  }
}
```

For example,

```json
{
  "ports": {
    "1": {
      "enabled": true,
      "poe": {
        "enabled": true,
        "mode": "802.3at"
      }
    },
    "2": {
      "enabled": true,
      "poe": {
        "enabled": false,
        "mode": "802.3at"
      }
    }
  }
}
```

## bin

There are a few utilities available on the switch that prove useful when interacting with a `switch.json` file.

- `switch_status` can be used to generate a `switch.json` file
- [`jshn`](https://openwrt.org/docs/guide-developer/jshn) is a JSON library for bash scripting

## daemon

The `configd` daemon will poll (with a 10 second wait) the `/etc/switch.json` file for changes and apply them to the switch using the `/click` interface.


## API

CGI scripts served by uhttpd on port 80 will respond to the following requests.

The base URL is

    http://{switch}/cgi-bin

Authentication is provided by PAM (i.e., the `root` user's login credentials).

- Get the current configuration

      GET /config 

- Upload a new configuration

      POST /config
      content-type: application/json
      
      {
          "ports": {
              "1": {. . .},
              "2": {. . .},
              . . . 
          }
      }
  
## UI

A graphical interface is available at http://{switch}.
Here, you will find the status and configuration options exposed in an easy-to-use management dashboard.

All of the configuration found in the sections above (except for the `click` filesystem) can be managed through the UI.

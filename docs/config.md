# switch configuration

This doc describes how a postmerkos switch is configured.

> **NOTE**: If you are an end-user looking to manage your switch, we recommend the [UI](#UI).
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

An example, truncated to include only 2 ports:

```json
{
  "ports": {
    "1": {
      "enabled": true,
      "poe": {
        "enabled": true,
        "mode": "at"
      }
    },
    "2": {
      "enabled": true,
      "poe": {
        "enabled": false,
        "mode": "af"
      }
    }
  }
}
```

## bin

The following binaries are related to device configuration and status.

### `configd`


The `configd` daemon will poll (with a 1 second wait) the `/etc/switch.json` file (and create it, if one doesn't exist) for changes and apply them to the switch using the `/click` interface.


### `status`

The `status` command will report read-only values in json format.

It's the logic behind the `/status` API documented below.


## API

CGI scripts served by uhttpd on port 80 will respond to the following requests.

The base URL is

    http://{switch}/cgi-bin

Authentication is provided by PAM (i.e., the `root` user's login credentials).

### config

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

### status

- Get the current status

      GET /status

  An example response:

  ```json
  {
    "datetime": "2022-01-21T18:58:15.837Z",
    "temperature": {
      "system": [
        40,
        33
      ],
      "poe": [
        35.297225952148438,
        41.241744995117188,
        33.315719604492188,
        31.994716644287109
      ]
    },
    "ports": {
      "1": {
        "link": {
          "established": true,
          "speed": 1000
        },
        "poe": {
          "power": 6
        }
      }
    }
  }
  ```

## UI

A graphical interface is available at http://{switch}.
Here, you will find the status and configuration options exposed in an easy-to-use management dashboard.

All of the configuration found in the sections above (except for the `click` filesystem) can be managed through the UI.

# Configd

`configd` is a daemon that

  - watches an `/etc/switch.json` file and applies changes out to the click filesystem
  - listens for websocket connections to expose device status and configuration

## dev

> **NOTE**: will change post-merge

To build locally:

```bash
devenv shell
build configd
```

To execute on a device called `switch`:

```bash
# TODO: this lib should eventually be embedded in newer firmware versions
scp -O build/buildroot/output/target/usr/lib/libwebsockets.so* switch:/tmp/

scp -O build/buildroot/output/target/bin/configd switch:/tmp/
ssh switch LD_LIBRARY_PATH=/tmp /tmp/configd
```

then start the [postmerkos-ui](https://github.com/hall/postmerkos-ui/blob/main/CONTRIBUTING.md)

```bash
SWITCH_HOST=switch npm run dev
```

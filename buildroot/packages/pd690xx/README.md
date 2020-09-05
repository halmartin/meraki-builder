# Info

The `pd690xx` program does basic PoE management on Meraki MS220-P series switches.

## Usage

```
Usage: pd690xx [OPTIONS]
Options:
        -b [BUS]        Selects a different I2C bus (default 1)
        -d [PORT]       Disable PoE on port PORT
        -e [PORT]       Enable PoE on port PORT
        -h              Program usage
        -p [PORT]       Print PoE power consumption (system total, or on port PORT)
        -r [PORT]       Reset PoE on port PORT
        -t              Display average junction temperature (deg C) of pd690xx
```

**Example 1**: Disable PoE on port 2
```
$ pd690xx -d 2
Port 2 PoE disabled
```

**Example 2**: Get total power delivered through PoE across all ports
```
$ pd690xx -p
Total: 2.3 W
```

## Not implemented

* `-b` (switch the I2C bus used to communicate with the pd690xx; on larger switches `/dev/i2c-2` is used)

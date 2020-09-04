# Info

The `pd690xx` program does basic PoE management on Meraki MS220-P series switches.

## Usage

* `-d #` (disable PoE on port number #)
* `-e #` (enable PoE on port number #)
* `-p` (get total PoE delivered power; Watts)

**Example 1**: Disable PoE on port 2
```
$ pd690xx -d 2
Port 2 PoE disabled
```

**Example 2**: Get total power delivered through PoE across all ports
```
$ pd690xx -p
2.3 W
```

## Not implemented

* `-b` (switch the I2C bus used to communicate with the pd690xx; on larger switches `/dev/i2c-2` is used)
* `-t` (supposed to get the pd690xx temperature, but the formula to convert to Celsius in the datasheet makes ZERO SENSE)

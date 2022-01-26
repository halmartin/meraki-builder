# Info

The `pd690xx` program does basic PoE management on Meraki MS220-P series switches.

## Usage

```
Usage: pd690xx [OPTIONS]
v0.31
Options:
        -d <PORT>       Disable PoE on port PORT
        -e <PORT>       Enable PoE on port PORT
        -f <PORT>       Force PoE on port PORT
        -h              Program usage
        -l              List all port statuses
        -p [PORT]       Print PoE power consumption (system total, or on port PORT)
        -r <PORT>       Reset PoE on port PORT
        -s              Display PoE voltage
        -t              Display average junction temperature (deg C) of pd690xx
```

**Example 1**: Disable PoE on port 2
```
$ pd690xx -d 2
Port 2 PoE disabled
```

**Example 2**: List the status of each PoE port (the MS220-8P will display 12 ports, as that is the number of ports each `pd690xx` supports)
```
$ pd690xx -l
Port    Status          Type    Priority        Power
1       Enabled         at      Critical        0.0
2       Enabled         at      Critical        3.0
3       Enabled         at      Critical        0.0
4       Enabled         at      Critical        0.0
5       Enabled         at      Critical        0.0
6       Enabled         at      Critical        0.0
7       Enabled         at      Critical        0.0
8       Enabled         at      Critical        0.0
9       Enabled         at      Critical        0.0
10      Enabled         at      Critical        0.0
11      Enabled         at      Critical        0.0
12      Enabled         at      Critical        0.0
```

**Example 3**: Get PoE voltage at switch
```
$ pd690xx -s
54.9 V
55.1 V
55.1 V
54.9 V
```

Note: the MS220-8P will display one voltage, 24 port models will display two voltages, and 48 port models will display four voltages

**Example 4**: Get `pd690xx` temperature
```
$ pd690xx -t
45.2 C
46.5 C
46.5 C
47.2 C
```

Note: the MS220-8P will display one temperature, 24 port models will display two temperatures, and 48 port models will display four temperatures

## Not implemented

* Ability to change port configuration between 802.3at and 802.3af

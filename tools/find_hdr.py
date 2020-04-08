#!/usr/bin/env python3

def find_header(opts):
    with open(opts.firmware, 'rb') as firm_file:
        # shouldn't read the entire file into memory
        # but for the Meraki switch this will be <20MB
        f_data = firm_file.read()
    if opts.gzip:
        return f_data.find(b'\x1f\x8b\x08\x00\x00\x00\x00')
    elif opts.xz:
        # from 2.1.1.1 in https://tukaani.org/xz/xz-file-format.txt
        return f_data.find(b'\xfd\x37\x7a\x58\x5a\x00')
    elif opts.st1f:
        # "attributes"
        return f_data.find(b'\x61\x74\x74\x72\x69\x62\x75\x74\x65\x73')+26

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("firmware")
    parser.add_argument("-g", "--gzip", action="store_true", help="Search for GZIP header")
    parser.add_argument("-x", "--xz", action="store_true", help="Search for XZ header")
    parser.add_argument("-1f", "--st1f", action="store_true", help="Search for boot1 footer")
    args = parser.parse_args()
    loc = find_header(args)
    print(loc)

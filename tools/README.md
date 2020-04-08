# Compile

`gcc find_hdr.c -o find_hdr`

# Usage

`find_hdr` is used by the extraction scripts to find gzip/XZ headers (or an XZ footer). The extract scripts expect `find_hdr` be in your `PATH`

It returns the byte offset you use with dd to extract the archive. This avoids hard-coded offsets in the extraction script.

# Python

There is also a python version included (`find_hdr.py`), however I mainly used the C version so there may be bugs in the python version.

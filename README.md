NMEA0183 MJP JM3 VDR monitor
============================

VDR dara monitoring
-------------------
A simple monitor for MJP VDR data monitoring.

Compile
-------
gcc nmea0183mon.c -o nmon

Verified on environment
-----------------------
Tested on a RPi 3, Rasperian Linux.

Usage
-----
| Command                       | Description |
|-------------------------------|-------------|
| ./nmon			| Defaults reading from /dev/ttyUSB0 |
| ./nmon /dev/ttyUSB1		| Reading from /dev/ttyUSB1 |
| ./nmon /dev/ttyUSB0 log	| Readling from /dev/ttyUSB0 and output nmea records data log to file. |
<br />
When log is given, the input device has to be given.<br />


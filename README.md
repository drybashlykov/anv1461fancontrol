# anv1461fancontrol
Fan control daemon for Acer Nitro V14 (ANV 14-61) on Linux

Couldn't get NBFC to work so wrote this thing

## Build
```
gcc -O2 -o ec-fand ec-fand.c
sudo setcap cap_sys_rawio+ep ec-fand
```

## Warning
This doesn't even have a lock preventing more than 1 instance of the daemon running and if that does happen, it could cause the EC to freak out.

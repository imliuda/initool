# Introduction
A tool for manage ini format configration file, it can add/update/delete/get config options in global area and particular section.
it surport duplicated option key.
# compile
```shell
gcc initool.c -o initool
```
# Usage
```shell
initool option filename [-s section] name [value] 

Accepted option include:
-a add option
-d delete option
-u update option
-g get option

Section option:
-s section name, optional
Delimiter option:
-f default is "="
```
# Format
follow is an example code similar to systemd's service configuration.
```shell
# this is a comment line
; this is alse a comment line
; "#" in next line is not a comment, but part of Hello's value.
Hello=World # not a comment

; option values can't expand lines, the Noexpand's value is "Hello\"
Noexpand=Hello\
world

[Unit]
Description=Example of initool

[Service]
Restart=always
ExecStart=/bin/ls
ExecStart=/bin/pwd

[Install]
WantedBy=multi-user.target
```
## Examples
delete ExecStart option in Service section of above code, this will delete all ExecStart option in Service section.
```shell
initool -d config.ini -s Service ExecStart
```
add option to global area or particular section, without "-s" option, initool will add new option to the begin of config file, and become a global option.
```shell
initool -a config.ini Newopt newopt_value
```
update option in Service section.
```shell
initool -u config.ini -s Service Restart no
```
add two ExecStart options, this will output "/bin/ls /bin/pwd "
```shell
initool -g config.ini -s Service ExecStart
```
this tool is very simple, but there may be some bug, including allocated memery not free. Before using it, please read the source!

#!/bin/sh

pid=$(pgrep -n "hl_linux")
lib="$(pwd)/libahc.so"

if [ $(id -u) -ne 0 ]
then
	exit 1
fi

if [ -z "$pid" ]
then
	exit 1
fi

if [ ! -f "$lib" ]
then
	exit 1
fi

if grep -q "$lib" "/proc/$pid/maps"
then
	gdb -n -q -batch \
	    -ex "attach $pid" \
	    -ex "set \$dlopen = (void *(*)(char *, int))dlopen" \
	    -ex "set \$dlclose = (int (*)(void *))dlclose" \
	    -ex "set \$lib = \$dlopen(\"$lib\", 6)" \
	    -ex "call \$dlclose(\$lib)" \
	    -ex "call \$dlclose(\$lib)" \
	    -ex "detach" \
	    -ex "quit"
fi

gdb -n -q -batch \
    -ex "attach $pid" \
    -ex "set \$dlopen = (void *(*)(char *, int))dlopen" \
    -ex "call \$dlopen(\"$lib\", 1)" \
    -ex "detach" \
    -ex "quit"

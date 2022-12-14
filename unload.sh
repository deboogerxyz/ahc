#!/bin/sh

pid=$(pgrep -n "hl_linux")
lib="$(pwd)/libahc.so"

if [ $(id -u) -ne 0 ]
then
	echo "requires root privileges"
	exit 1
fi

if [ -z "$pid" ]
then
	echo "target process not found"
	exit 1
fi

if ! grep -q "$lib" "/proc/$pid/maps"
then
	echo "shared object not loaded"
	exit 1
fi

gdb -n -q -batch \
    -ex "attach $pid" \
    -ex "set \$dlopen = (void *(*)(char *, int))dlopen" \
    -ex "set \$dlclose = (int (*)(void *))dlclose" \
    -ex "set \$lib = \$dlopen(\"$lib\", 6)" \
    -ex "call \$dlclose(\$lib)" \
    -ex "call \$dlclose(\$lib)" \
    -ex "detach" \
    -ex "quit"

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500
#CFLAGS   = -g -m32 -fPIC -std=c89 -pedantic -Wall -Wno-deprecated-declarations -O0 ${CPPFLAGS}
CFLAGS   = -m32 -std=c89 -fPIC -pedantic -Wall -Wno-deprecated-declarations -Os ${CPPFLAGS}
LDFLAGS  = -m32 -shared

# compiler and linker
CC = cc

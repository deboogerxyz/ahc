#include "parsemsg.h"

static int flags;
static int size;
static int maxsize;
static int readsize;
static int readcount;
static char *data;

void
parsemsg_begin(void *buf, int sizearg, int flagsarg)
{
	flags = flagsarg;
	size = sizearg;
	maxsize = 0;
	readsize = 0;
	readcount = 0;
	data = buf;
}

int
parsemsg_canread(void)
{
	if (!maxsize)
		return readcount + readsize <= size;
	
	return readcount + readsize <= size && size <= maxsize;
}

unsigned char
parsemsg_readbyte(void)
{
	unsigned char byte;

	readsize = 1;

	if (!parsemsg_canread())
		return PARSEMSG_INVALID;

	byte = data[readcount];
	readcount += readsize;

	return byte;
}

char *
parsemsg_readstr(void)
{
	int i, len, byte;
	static char str[8192] = {0};

	readsize = 1;

	if (!parsemsg_canread())
		return 0;

	len = flags & PARSEMSG_USERMSG ? 2048 : (flags & PARSEMSG_NETMSG ? 8192 : 2048);
	for (i = 0; i < len - 1; i++) {
		byte = parsemsg_readbyte();

		if (!byte || byte == PARSEMSG_INVALID)
			break;

		str[i] = byte;
	}

	str[i] = '\0';
	return str;
}

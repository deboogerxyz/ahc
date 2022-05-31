enum {
	PARSEMSG_INVALID = -1,
	PARSEMSG_NONE = 0,
	PARSEMSG_USERMSG = (1 << 0),
	PARSEMSG_NETMSG = (1 << 1),
	PARSEMSG_READONLY = (1 << 2)
};

void parsemsg_begin(void *buf, int sizearg, int flagsarg);
int parsemsg_canread(void);
unsigned char parsemsg_readbyte(void);
char *parsemsg_readstr(void);

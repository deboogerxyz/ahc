typedef struct {
	float x, y, z;
} Vector;

enum {
	IN_ATTACK = (1 << 0),
	IN_JUMP = (1 << 1),
	IN_DUCK = (1 << 2),
	IN_FORWARD = (1 << 3),
	IN_BACK = (1 << 4),
	IN_USE = (1 << 5),
	IN_CANCEL = (1 << 6),
	IN_LEFT = (1 << 7),
	IN_RIGHT = (1 << 8),
	IN_MOVELEFT = (1 << 9),
	IN_MOVERIGHT = (1 << 10),
	IN_ATTACK2 = (1 << 11),
	IN_RUN = (1 << 12),
	IN_RELOAD = (1 << 13),
	IN_ALT1 = (1 << 14),
	IN_SCORE = (1 << 15)
};

typedef struct {
	PAD(0x3)
	Vector viewangles;
	float forwardmove;
	float sidemove;
	float upmove;
	PAD(0x1)
	unsigned short buttons;
	PAD(0x12)
} UserCmd;

typedef struct UserMsg UserMsg;

struct UserMsg {
	int msg;
	int size;
	char name[0x10];
	UserMsg *next;
	int (*fn)(char *name, int size, void *buf);
};

enum {
	MOVETYPE_NONE,
	MOVETYPE_WALK = 3,
	MOVETYPE_STEP,
	MOVETYPE_FLY,
	MOVETYPE_TOSS,
	MOVETYPE_PUSH,
	MOVETYPE_NOCLIP,
	MOVETYPE_FLYMISSILE,
	MOVETYPE_BOUNCE,
	MOVETYPE_BOUNCEMISSILE,
	MOVETYPE_FOLLOW,
	MOVETYPE_PUSHSTEP
};

typedef struct {
	PAD(12)
	int msgnum;
	PAD(72)
	int movetype;
	PAD(248)
} EntityState;

typedef struct {
	int i;
	int player;
	PAD(2 * sizeof(EntityState))
	EntityState curstate;
	PAD(1860)
	Vector origin;
} Entity;

typedef struct {
	PAD(0x18)
	void (*localplayerviewheight)(Vector *);
	PAD(0x40)
} EventAPI;

typedef struct {
	PAD(0x8C)
	void (*setviewangles)(Vector *angles);
	int (*getmaxclients)(void);
	PAD(0xC)
	void (*con_printf)(char *fmt, ...);
	PAD(0x28)
	Entity *(*getlocalplayer)(void);
	PAD(0x4)
	Entity *(*getentitybyindex)(int i);
	PAD(0x78)
	EventAPI *eventapi;
} Engine;

typedef float Matrix3x4[3][4];

typedef struct {
	PAD(0x18)
	Entity *(*getcurrententity)(void);
	PAD(36)
	Matrix3x4 **(*getbonetransform)(void);
} EngineStudio;

typedef struct {
	PAD(0x4C)
	void (*rendermodel)(void *this);
	void (*renderfinal)(void *this);
} StudioModelRenderer;

typedef struct {
	Vector vieworigin;
	Vector viewangles;
	Vector forward;
	Vector right;
	Vector up;
	PAD(0x64)
	Vector punchangle;
} RefParams;

typedef struct {
	PAD(0xC)
	int (*hudredraw)(float time, int intermission);
	PAD(0x28)
	void (*createmove)(float frametime, UserCmd *cmd, int active);
	int (*is3rdperson)(void);
	void (*cameraoffset)(Vector *offset);
	PAD(0x8)
	void (*calcrefdef)(RefParams *params);
} Client;

enum {
	FL_FLY = (1 << 0),
	FL_SWIM = (1 << 1),
	FL_CONVEYOR = (1 << 2),
	FL_CLIENT = (1 << 3),
	FL_INWATER = (1 << 4),
	FL_MONSTER = (1 << 5),
	FL_GODMODE = (1 << 6),
	FL_NOTARGET = (1 << 7),
	FL_SKIPLOCALHOST = (1 << 8),
	FL_ONGROUND = (1 << 9),
	FL_PARTIALGROUND = (1 << 10),
	FL_WATERJUMP = (1 << 11),
	FL_FROZEN = (1 << 12),
	FL_FAKECLIENT = (1 << 13),
	FL_DUCKING = (1 << 14),
	FL_FLOAT = (1 << 15),
	FL_GRAPHED = (1 << 16)
};

typedef struct {
	PAD(56)
	Vector origin;
	PAD(24)
	Vector velocity;
	PAD(80)
	int flags;
} PlayerMove;

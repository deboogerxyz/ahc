#include <dlfcn.h>
#include <GL/gl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "parsemsg.h"
#include "util.h"
#include "sdk.h"

typedef void (*GlColor4f)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

static pthread_t pthread;

static void *hw;
static Engine *engine;
static Client *client, origclient;
static int (*origscreenfade)(char *name, int size, void *buf);
static int (*origteaminfo)(char *name, int size, void *buf);
static PlayerMove *playermove;
static UserMsg *clientusermsgs;
static EngineStudio *enginestudio;
static StudioModelRenderer *studiomodelrenderer, origstudiomodelrenderer;
static GlColor4f origglcolor4f;

static Entity *localplayer;
static Vector bones[64][128];
static int renderstate = -1;
static char playersteam[32][16];

static void *
getalignedaddr(void *addr)
{
	return (void *)((uintptr_t)addr - (uintptr_t)addr % getpagesize());
}

static void
getmodules(void)
{
	void *shutdownusermsgs, *clientdll;

	hw = dlopen("hw.so", RTLD_LAZY | RTLD_NOLOAD);

	engine = dlsym(hw, "cl_enginefuncs");
	client = dlsym(hw, "cl_funcs");
	playermove = *(PlayerMove **)dlsym(hw, "pmove");

	shutdownusermsgs = dlsym(hw, "CL_ShutDownUsrMessages");
	clientusermsgs = **(UserMsg ***)((char *)shutdownusermsgs + 0x5);

	enginestudio = dlsym(hw, "engine_studio_api");

	clientdll = *(void **)dlsym(hw, "hClientDLL");
	studiomodelrenderer = *(StudioModelRenderer **)dlsym(clientdll, "g_StudioRenderer");
	mprotect(getalignedaddr(studiomodelrenderer), getpagesize(), PROT_READ | PROT_WRITE);

	dlclose(hw);
}

static UserMsg *
findusermsg(char *name)
{
	UserMsg *usermsg;

	for (usermsg = clientusermsgs; usermsg; usermsg = usermsg->next)
		if (!strcmp(usermsg->name, name))
			return usermsg;

	return NULL;
}

static int
isalive(Entity *ent)
{
	return ent->curstate.movetype != MOVETYPE_TOSS && ent->curstate.movetype != MOVETYPE_NONE;
}

static void
bhop(UserCmd *cmd)
{
	static int wasonground = 0;

	if (!isalive(localplayer))
		return;

	if (!playermove)
		return;

	if (!(playermove->flags & FL_ONGROUND) && !wasonground)
		cmd->buttons &= ~IN_JUMP;

	wasonground = playermove->flags & FL_ONGROUND;
}

static float
vector_len2d(Vector v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

static Vector
vector_toang(Vector v)
{
	Vector a;

	a.x = RAD2DEG(atan2(-v.z, hypot(v.x, v.y)));
	a.y = RAD2DEG(atan2(v.y, v.x));
	a.z = 0;

	return a;
}

static float
vector_toang2d(Vector v)
{
	return RAD2DEG(atan2f(v.y, v.x));
}

static Vector
vector_fromang(Vector v)
{
	Vector a;

	a.x = cosf(DEG2RAD(v.x)) * cosf(DEG2RAD(v.y));
	a.y = cosf(DEG2RAD(v.x)) * sinf(DEG2RAD(v.y));
	a.z = -sinf(DEG2RAD(v.x));

	return a;
}

static Vector
vector_fromang2d(float ang)
{
	Vector a;

	a.x = cosf(DEG2RAD(ang));
	a.y = sinf(DEG2RAD(ang));
	a.z = 0;

	return a;
}

static void
faststop(UserCmd *cmd)
{
	float speed, dir;
	Vector negdir;

	if (cmd->buttons & (IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK))
		return;

	if (cmd->buttons & IN_JUMP)
		return;

	if (!playermove)
		return;

	if (!(playermove->flags & FL_ONGROUND))
		return;

	if (!isalive(localplayer))
		return;

	speed = vector_len2d(playermove->velocity);
	if (speed < 15)
		return;

	dir = cmd->viewangles.y - vector_toang2d(playermove->velocity);
	negdir = vector_fromang2d(dir);

	/* TODO: get cvars dynamically at runtime
	 * cl_forwardspeed = 400, cl_sidespeed = 400
	 */
	cmd->forwardmove = negdir.x * -400;
	cmd->sidemove = negdir.y * -400;
}

static float
angledeltarad(float a, float b)
{
	float delta;

	delta = isfinite(a - b) ? remainder(a - b, 360) : 0;

	if (a > b) {
		if (delta >= M_PI)
			delta -= M_PI * 2;
	} else {
		if (delta <= -M_PI)
			delta += M_PI * 2;
	}

	return delta;
}

static void
autostrafer(UserCmd *cmd)
{
	float speed, term, bestdelta, yaw, veldir, wishang, delta, movedir;

	if (!(cmd->buttons & IN_JUMP))
		return;

	if (!isalive(localplayer))
		return;

	if (!playermove)
		return;

	speed = vector_len2d(playermove->velocity);

	if (speed < 15)
		return;

	/* TODO: get cvars dynamically at runtime
	 * sv_airaccelerate = 10, sv_maxspeed = 320
	 */
	term = 10.0f / 320.0f * 100.0f / speed;
	if (term < 1 && term > -1)
		bestdelta = acosf(term);
	else
		return;

	yaw = DEG2RAD(cmd->viewangles.y);
	veldir = atan2f(playermove->velocity.y, playermove->velocity.x) - yaw;
	wishang = atan2f(-cmd->sidemove, cmd->forwardmove);
	delta = angledeltarad(veldir, wishang);

	movedir = delta < 0 ? veldir + bestdelta : veldir - bestdelta;

	/* TODO: get cvars dynamically at runtime
	 * cl_forwardspeed = 400, cl_sidespeed = 400
	 */
	cmd->forwardmove = cosf(movedir) * 400;
	cmd->sidemove = -sinf(movedir) * 400;
}

static Vector
vector_add(Vector a, Vector b)
{
	Vector v;

	v.x = a.x + b.x;
	v.y = a.y + b.y;
	v.z = a.z + b.z;

	return v;
}

static Vector
vector_sub(Vector a, Vector b)
{
	Vector v;

	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;

	return v;
}

static Vector
vector_norm(Vector v)
{
	Vector a;

	a.x = isfinite(v.x) ? remainder(v.x, 360) : 0;
	a.y = isfinite(v.y) ? remainder(v.y, 360) : 0;
	a.z = 0;

	return a;
}

static Vector
vector_calcang(Vector start, Vector end, Vector ang)
{
	Vector a, b;

	a = vector_sub(end, start);
	b = vector_sub(vector_toang(a), ang);

	return vector_norm(b);
}

static int
vector_isnull(Vector v)
{
	return v.x && v.y && v.z;
}

static Vector
vector_mul(Vector v, float f)
{
	Vector a;

	a.x = v.x * f;
	a.y = v.y * f;
	a.z = v.z * f;

	return a;
}

static int
isenemy(Entity *ent)
{
	if (playersteam[ent->i][0] == '\0')
		return 1;

	if (!strcmp(playersteam[ent->i], playersteam[localplayer->i]))
		return 0;

	return 1;
}

static Vector recoilangles = {0};

static void
aimbot(UserCmd *cmd)
{
	int maxclients, i, j;
	Vector viewheight, eyepos, viewangles, bonepos;
	Entity *ent;
	float fov, bestfov = 2.5f;
	Vector ang, bestang = {0};

	if (!(cmd->buttons & IN_ATTACK))
		return;

	if (!playermove)
		return;

	engine->eventapi->localplayerviewheight(&viewheight);
	eyepos = vector_add(playermove->origin, viewheight);
	viewangles = vector_add(cmd->viewangles, vector_mul(recoilangles, 2));

	maxclients = engine->getmaxclients();
	for (i = 1; i <= maxclients; i++) {
		ent = engine->getentitybyindex(i);
		if (!ent)
			continue;

		if (!ent->player || ent->i == localplayer->i || !isalive(ent) || !isenemy(ent))
			continue;

		if (ent->curstate.msgnum < localplayer->curstate.msgnum)
			continue;

		for (j = 0; j < 128; j++) {
			bonepos = bones[i][j];
			ang = vector_calcang(eyepos, bonepos, viewangles);
			fov = hypotf(ang.x, ang.y);

			if (fov < bestfov) {
				bestfov = fov;
				bestang = ang;
			}
		}
	}

	if (vector_isnull(bestang))
		return;

	bestang.x /= 15.0f;
	bestang.y /= 15.0f;
	bestang.z /= 15.0f;

	cmd->viewangles = vector_add(cmd->viewangles, bestang);

	engine->setviewangles(&cmd->viewangles);
}

static void
fixmovement(UserCmd *cmd, float yaw)
{
	float oldyaw, newyaw, delta, forward, side;

	oldyaw = yaw + (yaw < 0 ? 360 : 0);
	newyaw = cmd->viewangles.y + (cmd->viewangles.y < 0 ? 360 : 0);
	delta = newyaw < oldyaw ? fabsf(newyaw - oldyaw) : 360 - fabsf(newyaw - oldyaw);

	delta = 360 - delta;

	forward = cmd->forwardmove;
	side = cmd->sidemove;

	cmd->forwardmove = cos(DEG2RAD(delta)) * forward + cos(DEG2RAD(delta + 90)) * side;
	cmd->sidemove = sin(DEG2RAD(delta)) * forward + sin(DEG2RAD(delta + 90)) * side;
}

static void
createmove(float frametime, UserCmd *cmd, int active)
{
	Vector currentangles;

	origclient.createmove(frametime, cmd, active);

	currentangles = cmd->viewangles;

	localplayer = engine->getlocalplayer();

	faststop(cmd);
	autostrafer(cmd);
	bhop(cmd);
	aimbot(cmd);

	fixmovement(cmd, currentangles.y);
}

static int
screenfade(char *name, int size, void *buf)
{
	return 1;
}

static Vector
matrix3x4_origin(Matrix3x4 m)
{
	Vector v;

	v.x = m[0][3];
	v.y = m[1][3];
	v.z = m[2][3];

	return v;
}

static void
rendermodel(void *this)
{
	int i;
	Entity *ent;
	BoneMatrix *mat;

	ent = enginestudio->getcurrententity();

	mat = enginestudio->getbonetransform();
	for (i = 0; i < 128; i++)
		bones[ent->i][i] = matrix3x4_origin((*mat)[i]);

	if (!localplayer || !ent->player || ent->i == localplayer->i || !isalive(ent) || !isenemy(ent)) {
		origstudiomodelrenderer.rendermodel(this);
		return;
	}

	glDisable(GL_TEXTURE_2D);

	renderstate = 0;
	glDisable(GL_DEPTH_TEST);
	origstudiomodelrenderer.renderfinal(this);

	renderstate = 1;
	glEnable(GL_DEPTH_TEST);
	origstudiomodelrenderer.renderfinal(this);

	renderstate = -1;
	glEnable(GL_TEXTURE_2D);
}

static void
glcolor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	switch (renderstate) {
	case 0:
		r = 1, g = 0, b = 0;
		break;
	case 1:
		r = 1, g = 1, b = 0;
		break;
	}

	origglcolor4f(r, g, b, a);
}

static void
calcrefdef(RefParams *params)
{
	recoilangles = params->punchangle;

	/*
	params->punchangle.x = 0;
	params->punchangle.y = 0;
	params->punchangle.z = 0;
	*/

	origclient.calcrefdef(params);

	/*
	params->vieworigin = vector_sub(params->vieworigin, vector_mul(params->forward, 150));
	*/
}

static int
is3rdperson(void)
{
	return 1;
}

static int
teaminfo(char *name, int size, void *buf)
{
	int i;
	char *team;

	parsemsg_begin(buf, size, PARSEMSG_USERMSG);

	i = parsemsg_readbyte();
	team = parsemsg_readstr();

	strcpy(playersteam[i], team);

	return origteaminfo(name, size, buf);
}

static void *
threadfn(void *argv)
{
	struct timespec ts = {0, 100000000}; /* 100ms */

	while (!dlopen("platform/servers/serverbrowser_linux.so", RTLD_NOLOAD | RTLD_NOW))
		nanosleep(&ts, &ts); /* Wait for the game to fully load */

	getmodules();

	origclient = *client;

	client->createmove = createmove;

	origscreenfade = findusermsg("ScreenFade")->fn;
	findusermsg("ScreenFade")->fn = screenfade;

	origteaminfo = findusermsg("TeamInfo")->fn;
	findusermsg("TeamInfo")->fn = teaminfo;

	origstudiomodelrenderer = *studiomodelrenderer;

	studiomodelrenderer->rendermodel = rendermodel;

	origglcolor4f = *(GlColor4f *)dlsym(hw, "qglColor4f");
	*(GlColor4f *)dlsym(hw, "qglColor4f") = glcolor4f;

	client->calcrefdef = calcrefdef;
	/*
	client->is3rdperson = is3rdperson;
	*/

	return NULL;
}

__attribute__((constructor))
static int
onload(void)
{
	pthread_create(&pthread, NULL, threadfn, NULL);
	pthread_detach(pthread);

	return 0;
}

__attribute__((destructor))
static void
onunload(void)
{
	pthread_join(pthread, NULL);

	*(GlColor4f *)dlsym(hw, "qglColor4f") = origglcolor4f;
	*studiomodelrenderer = origstudiomodelrenderer;
	findusermsg("TeamInfo")->fn = origteaminfo;
	findusermsg("ScreenFade")->fn = origscreenfade;
	*client = origclient;
}

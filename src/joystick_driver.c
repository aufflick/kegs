/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_joystick_driver_c[] = "@(#)$KmKId: joystick_driver.c,v 1.7 2002-11-19 03:09:59-05 kadickey Exp $";

#include "defc.h"
#include <sys/time.h>

#ifdef __linux__
# include <linux/joystick.h>
#endif

#ifdef _WIN32
# include <windows.h>
# include <mmsystem.h>
#endif

extern int g_joystick_type;		/* in paddles.c */
extern int g_paddle_button[];
extern int g_paddle_val[];


const char *g_joystick_dev = "/dev/js0";	/* default joystick dev file */
#define MAX_JOY_NAME	128

int	g_joystick_fd = -1;
int	g_joystick_num_axes = 0;
int	g_joystick_num_buttons = 0;


#ifdef __linux__
void
joystick_init()
{
	char	joy_name[MAX_JOY_NAME];
	int	version;
	int	fd;
	int	i;

	fd = open(g_joystick_dev, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		printf("Unable to open joystick dev file: %s, errno: %d\n",
			g_joystick_dev, errno);
		printf("Defaulting to mouse joystick\n");
		return;
	}

	strcpy(&joy_name[0], "Unknown Joystick");
	version = 0x800;

	ioctl(fd, JSIOCGNAME(MAX_JOY_NAME), &joy_name[0]);
	ioctl(fd, JSIOCGAXES, &g_joystick_num_axes);
	ioctl(fd, JSIOCGBUTTONS, &g_joystick_num_buttons);
	ioctl(fd, JSIOCGVERSION, &version);

	printf("Detected joystick: %s [%d axes, %d buttons vers: %08x]\n",
		joy_name, g_joystick_num_axes, g_joystick_num_buttons,
		version);

	g_joystick_type = JOYSTICK_LINUX;
	g_joystick_fd = fd;
	for(i = 0; i < 4; i++) {
		g_paddle_val[i] = 280;
		g_paddle_button[i] = 1;
	}

	joystick_update();
}

/* joystick_update_linux() called from paddles.c.  Update g_paddle_val[] */
/*  and g_paddle_button[] arrays with current information */
void
joystick_update()
{
	struct js_event js;	/* the linux joystick event record */
	int	val;
	int	num;
	int	type;
	int	ret;
	int	len;
	int	i;

	/* suck up to 20 events, then give up */
	len = sizeof(struct js_event);
	for(i = 0; i < 20; i++) {
		ret = read(g_joystick_fd, &js, len);
		if(ret != len) {
			/* just get out */
			return;
		}
		type = js.type & ~JS_EVENT_INIT;
		val = js.value;
		num = js.number & 3;		/* clamp to 0-3 */
		switch(type) {
		case JS_EVENT_BUTTON:
			g_paddle_button[num] = val;
			break;
		case JS_EVENT_AXIS:
			/* val is -32767 to +32767, convert to 0->280 */
			/* want just 255, but go a little over for robustness*/
			g_paddle_val[num] = ((val + 32767) * 9) >> 11;
			break;
		}
	}
}

void
joystick_update_button()
{
}

#else	/* !__linux__ */

# ifdef _WIN32
void
joystick_init()
{
	JOYINFO info;
	JOYCAPS joycap;
	MMRESULT ret1, ret2;
	int	i;

	//	Check that there is a joystick device
	if(joyGetNumDevs() <= 0) {
		printf("--No joystick hardware detected\n");
		return;
	}

	//	Check that at least joystick 1 or joystick 2 is available
	ret1 = joyGetPos(JOYSTICKID1, &info);
	ret2 = joyGetDevCaps(JOYSTICKID1, &joycap, sizeof(joycap));
	if(ret1 == JOYERR_NOERROR && ret2 == JOYERR_NOERROR) {
		g_joystick_type = JOYSTICK_WIN32_1;
		printf("--Joystick #1 = %s\n", joycap.szPname);
	} else {
		ret1 = joyGetPos(JOYSTICKID2, &info);
		ret2 = joyGetDevCaps(JOYSTICKID2, &joycap, sizeof(joycap));
		if(ret1 == JOYERR_NOERROR && ret2 == JOYERR_NOERROR) {
			g_joystick_type = JOYSTICK_WIN32_2;
			printf("--Joystick #2 = %s\n", joycap.szPname);
		} else {
			printf("No joysticks found...\n");
			g_joystick_type = JOYSTICK_MOUSE;
			return;
		}
	
	}

	for(i = 0; i < 4; i++) {
		g_paddle_val[i] = 280;
		g_paddle_button[i] = 1;
	}

	joystick_update();
}

void
joystick_update()
{
	JOYCAPS joycap;
	JOYINFO info;
	UINT	id;
	MMRESULT ret1, ret2;

	id = JOYSTICKID1;
	if(g_joystick_type == JOYSTICK_WIN32_2) {
		id = JOYSTICKID2;
	}

	ret1 = joyGetDevCaps(id, &joycap, sizeof(joycap));
	ret2 = joyGetPos(id, &info);
	if(ret1 == JOYERR_NOERROR && ret2 == JOYERR_NOERROR) {
		g_paddle_val[0] = (info.wXpos - joycap.wXmin) * 280 /
						(joycap.wXmax - joycap.wXmin);
		g_paddle_val[1] = (info.wYpos - joycap.wYmin) * 280 /
						(joycap.wYmax - joycap.wYmin);
		g_paddle_button[0] = ((info.wButtons & JOY_BUTTON1) ? 1 : 0);
		g_paddle_button[1] = ((info.wButtons & JOY_BUTTON2) ? 1 : 0);
	}
}

void
joystick_update_button()
{
	JOYINFOEX info;
	UINT id;

	id = JOYSTICKID1;
	if(g_joystick_type == JOYSTICK_WIN32_2) {
		id = JOYSTICKID2;
	}

	info.dwSize = sizeof(JOYINFOEX);
	info.dwFlags = JOY_RETURNBUTTONS;
	if(joyGetPosEx(id, &info) == JOYERR_NOERROR) {
		g_paddle_button[0] = ((info.dwButtons & JOY_BUTTON1) ? 1 : 0);
		g_paddle_button[1] = ((info.dwButtons & JOY_BUTTON2) ? 1 : 0);
	}
}
# else

/* stubs for the routines */
void
joystick_init()
{
	printf("No joy with joystick\n");
}

void
joystick_update()
{
}

void
joystick_update_button()
{
}

# endif		/* !WIN32 */
#endif

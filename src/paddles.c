/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_paddles_c[] = "@(#)$KmKId: paddles.c,v 1.7 2004-03-23 17:28:06-05 kentd Exp $";

#include "defc.h"

extern int g_mouse_fifo_x[];	/* from adb.c */
extern int g_mouse_fifo_y[];

double	g_paddle_trig_dcycs = 0.0;
int	g_swap_paddles = 0;
int	g_invert_paddles = 0;

int	g_joystick_type = JOYSTICK_MOUSE;

int	g_paddle_button[4] = { 0, 0, 0, 0 };
		/* g_paddle_button[0] = button 0, etc */

int	g_paddle_val[4] = { 0, 0, 0, 0 };
		/* g_paddle_val[0]: Joystick X coord, [1]:Y coord */


void
paddle_trigger(double dcycs)
{
	/* Called by read/write to $c070 */
	g_paddle_trig_dcycs = dcycs;

	/* Determine what all the paddle values are right now */

	switch(g_joystick_type) {
	case JOYSTICK_MOUSE:
		paddle_trigger_mouse(dcycs);
		break;
	case JOYSTICK_LINUX:
	case JOYSTICK_WIN32_1:
	case JOYSTICK_WIN32_2:
		joystick_update();
	}
}

void
paddle_trigger_mouse(double dcycs)
{
	int	val_x;
	int	val_y;

	val_x = 0;
	/* mous_phys_x is 0->560, convert that to 0-255 cyc */
	/* so, mult by 117 then divide by 256 */
	if(g_mouse_fifo_x[0] > BASE_MARGIN_LEFT) {
		val_x = (g_mouse_fifo_x[0] - BASE_MARGIN_LEFT) * 117;
		val_x = val_x >> 8;
	}

	/* mous_phys_y is 0->384, convert that to 0-255 cyc */
	/* so, mult by 170 then divide by 256 (shift right by 8) */
	val_y = 0;
	if(g_mouse_fifo_y[0] > BASE_MARGIN_TOP) {
		val_y = ((g_mouse_fifo_y[0] - BASE_MARGIN_TOP) * 170) >> 8;
	}

	if(val_x > 280) {
		val_x = 280;
	}
	if(val_y > 280) {
		val_y = 280;
	}

	g_paddle_val[0] = val_x;
	g_paddle_val[1] = val_y;
	g_paddle_val[2] = 255;
	g_paddle_val[3] = 255;
	g_paddle_button[2] = 1;
	g_paddle_button[3] = 1;
}

int
read_paddles(int paddle, double dcycs)
{
	double	trig_dcycs;
	int	val;

	/* This routine is called by any read to $c064-$c067 */
	if(g_swap_paddles) {
		paddle = paddle ^ 1;
	}

	paddle = paddle & 3;

	val = g_paddle_val[paddle];

	if(g_invert_paddles) {
		val = 255 - val;
	}

	/* convert 0->255 into 0->2816.0 cycles (the paddle delay const) */
	trig_dcycs = g_paddle_trig_dcycs + (val * 11.0);

	if(dcycs < trig_dcycs) {
		return 0x80;
	} else {
		return 0x00;
	}
}


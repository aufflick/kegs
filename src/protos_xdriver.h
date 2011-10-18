/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_protos_x_h[] = "@(#)$KmKId: protos_xdriver.h,v 1.18 2002-11-19 03:10:38-05 kadickey Exp $";

/* END_HDR */

/* xdriver.c */
int main(int argc, char **argv);
void x_update_physical_colormap(void);
void show_xcolor_array(void);
int my_error_handler(Display *display, XErrorEvent *ev);
void xdriver_end(void);
void show_colormap(char *str, Colormap cmap, int index1, int index2, int index3);
void dev_video_init(void);
Visual *x_try_find_visual(int depth, int screen_num, XVisualInfo **visual_list_ptr);
void x_set_mask_and_shift(word32 x_mask, word32 *mask_ptr, int *shift_left_ptr, int *shift_right_ptr);
int xhandle_shm_error(Display *display, XErrorEvent *event);
int get_shm(Kimage *kimage_ptr);
void get_ximage(Kimage *kimage_ptr);
void update_status_line(int line, const char *string);
void redraw_status_lines(void);
void check_input_events(void);
void handle_keysym(XEvent *xev_in);
int x_keysym_to_a2code(int keysym, int is_up);
void x_update_modifier_state(int state);
void x_auto_repeat_on(int must);
void x_auto_repeat_off(int must);


/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_protos_mac_h[] = "@(#)$KmKId: protos_macdriver.h,v 1.6 2004-03-23 17:27:31-05 kentd Exp $";

/* END_HDR */

/* macdriver.c */
pascal OSStatus quit_event_handler(EventHandlerCallRef call_ref, EventRef event, void *ignore);
void show_alert(const char *str1, const char *str2, const char *str3, int num);
pascal OSStatus my_cmd_handler(EventHandlerCallRef handlerRef, EventRef event, void *userdata);
void update_window(void);
void show_event(UInt32 event_class, UInt32 event_kind, int handled);
pascal OSStatus my_win_handler(EventHandlerCallRef handlerRef, EventRef event, void *userdata);
pascal OSStatus dummy_event_handler(EventHandlerCallRef call_ref, EventRef in_event, void *ignore);
void mac_update_modifiers(word32 state);
void mac_warp_mouse(void);
void check_input_events(void);
void temp_run_application_event_loop(void);
int main(int argc, char *argv[]);
void x_update_color(int col_num, int red, int green, int blue, word32 rgb);
void x_update_physical_colormap(void);
void show_xcolor_array(void);
void xdriver_end(void);
void x_get_kimage(Kimage *kimage_ptr);
void dev_video_init(void);
void x_redraw_status_lines(void);
void x_push_kimage(Kimage *kimage_ptr, int destx, int desty, int srcx, int srcy, int width, int height);
void x_push_done(void);
void x_auto_repeat_on(int must);
void x_auto_repeat_off(int must);
void x_hide_pointer(int do_hide);


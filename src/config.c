/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2003 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_config_c[] = "@(#)$KmKId: config.c,v 1.30 2003-11-21 16:38:53-05 kentd Exp $";

#include "defc.h"
#include <stdarg.h>
#include "config.h"
#include <dirent.h>

extern int Verbose;
extern word32 g_vbl_count;
extern Iwm iwm;

extern int g_track_bytes_35[];
extern int g_track_nibs_35[];
extern int g_apple35_sel;

extern int g_cur_a2_stat;
extern byte *g_slow_memory_ptr;
extern double g_cur_dcycs;

extern word32 g_adb_repeat_vbl;

extern int g_limit_speed;
extern int g_force_depth;
extern int g_raw_serial;
extern int g_serial_out_masking;
extern word32 g_mem_size_exp;
extern int g_video_line_update_interval;

extern int g_screen_index[];
extern word32 g_full_refresh_needed;
extern word32 g_a2_screen_buffer_changed;
extern int g_a2_new_all_stat[];
extern int g_new_a2_stat_cur_line;

extern int g_key_down;
extern const char g_kegs_version_str[];

int g_config_control_panel = 0;
char g_config_kegs_name[256];
char g_cfg_cwd_str[CFG_PATH_MAX] = { 0 };

int g_config_kegs_auto_update = 1;
int g_config_kegs_update_needed = 0;

const char *g_config_kegs_name_list[] = {
		"config.kegs", "kegs_conf", ".config.kegs", 0
};

int	g_highest_smartport_unit = -1;
int	g_reparse_delay = 0;

byte g_save_text_screen_bytes[0x800];
word32 g_save_cur_a2_stat = 0;
char g_cfg_printf_buf[CFG_PRINTF_BUFSIZE];
char	g_config_kegs_buf[CONF_BUF_LEN];

word32	g_cfg_vbl_count = 0;

int	g_cfg_curs_x = 0;
int	g_cfg_curs_y = 0;
int	g_cfg_curs_inv = 0;
int	g_cfg_curs_mousetext = 0;

#define CFG_MAX_OPTS	16
#define CFG_OPT_MAXSTR	100

int g_cfg_opts_vals[CFG_MAX_OPTS];
char g_cfg_opts_strs[CFG_MAX_OPTS][CFG_OPT_MAXSTR];
char g_cfg_opt_buf[CFG_OPT_MAXSTR];

#define MAX_PARTITION_BLK_SIZE		65536

extern Cfg_menu g_cfg_main_menu[];

#define KNMP(a)		&a, #a, 0

Cfg_menu g_cfg_disk_menu[] = {
{ "Disk Configuration", g_cfg_disk_menu, 0, 0, CFGTYPE_MENU },
{ "s5d1 = ", 0, 0, 0, CFGTYPE_DISK + 0x5000 },
{ "s5d2 = ", 0, 0, 0, CFGTYPE_DISK + 0x5010 },
{ "", 0, 0, 0, 0 },
{ "s6d1 = ", 0, 0, 0, CFGTYPE_DISK + 0x6000 },
{ "s6d2 = ", 0, 0, 0, CFGTYPE_DISK + 0x6010 },
{ "", 0, 0, 0, 0 },
{ "s7d1 = ", 0, 0, 0, CFGTYPE_DISK + 0x7000 },
{ "s7d2 = ", 0, 0, 0, CFGTYPE_DISK + 0x7010 },
{ "s7d3 = ", 0, 0, 0, CFGTYPE_DISK + 0x7020 },
{ "s7d4 = ", 0, 0, 0, CFGTYPE_DISK + 0x7030 },
{ "s7d5 = ", 0, 0, 0, CFGTYPE_DISK + 0x7040 },
{ "s7d6 = ", 0, 0, 0, CFGTYPE_DISK + 0x7050 },
{ "s7d7 = ", 0, 0, 0, CFGTYPE_DISK + 0x7060 },
{ "s7d8 = ", 0, 0, 0, CFGTYPE_DISK + 0x7070 },
{ "s7d9 = ", 0, 0, 0, CFGTYPE_DISK + 0x7080 },
{ "s7d10 = ", 0, 0, 0, CFGTYPE_DISK + 0x7090 },
{ "s7d11 = ", 0, 0, 0, CFGTYPE_DISK + 0x70a0 },
{ "", 0, 0, 0, 0 },
{ "Back to Main Config", g_cfg_main_menu, 0, 0, CFGTYPE_MENU },
{ 0, 0, 0, 0, 0 },
};

Cfg_menu g_cfg_main_menu[] = {
{ "KEGS Configuration", g_cfg_main_menu, 0, 0, CFGTYPE_MENU },
{ "Disk Configuration", g_cfg_disk_menu, 0, 0, CFGTYPE_MENU },
{ "Force X-windows display depth", KNMP(g_force_depth), CFGTYPE_INT },
{ "Auto-update config.kegs,0,Manual,1,Immediately",
		KNMP(g_config_kegs_auto_update), CFGTYPE_INT },
{ "Speed,0,Unlimited,1,1.0MHz,2,2.8MHz,3,8.0MHz (Zip)",
		KNMP(g_limit_speed), CFGTYPE_INT },
{ "Expansion Mem Size,0,0MB,0x100000,1MB,0x200000,2MB,0x300000,3MB,"
	"0x400000,4MB,0x600000,6MB,0x800000,8MB,0xa00000,10MB,0xc00000,12MB,"
	"0xe00000,14MB", KNMP(g_mem_size_exp), CFGTYPE_INT },
{ "Serial Ports,0,Only use sockets 6501-6502,1,Use real ports if avail",
		KNMP(g_raw_serial), CFGTYPE_INT },
{ "Serial Output,0,Send full 8-bit data,1,Mask off high bit",
		KNMP(g_serial_out_masking), CFGTYPE_INT },
{ "3200 Color Enable,0,Auto (Full if fast enough),1,Full (Update every line),"
	"8,Off (Update video every 8 lines)",
		KNMP(g_video_line_update_interval), CFGTYPE_INT },
{ "Dump text screen to file", (void *)cfg_text_screen_dump, 0, 0, CFGTYPE_FUNC},
{ "", 0, 0, 0, 0 },
{ "Save changes to config.kegs", (void *)config_write_config_kegs_file, 0, 0, 
		CFGTYPE_FUNC },
{ "", 0, 0, 0, 0 },
{ "Exit Config (or press F4)", (void *)cfg_exit, 0, 0, CFGTYPE_FUNC },
{ 0, 0, 0, 0, 0 },
};


#define CFG_MAX_DEFVALS	128
Cfg_defval g_cfg_defvals[CFG_MAX_DEFVALS];
int g_cfg_defval_index = 0;

int g_cfg_slotdrive = -1;
int g_cfg_select_partition = -1;
char g_cfg_tmp_path[CFG_PATH_MAX];
char g_cfg_file_path[CFG_PATH_MAX];
char g_cfg_file_cachedpath[CFG_PATH_MAX];
char g_cfg_file_cachedreal[CFG_PATH_MAX];
char g_cfg_file_curpath[CFG_PATH_MAX];
char g_cfg_file_shortened[CFG_PATH_MAX];
char g_cfg_file_match[CFG_PATH_MAX];

Cfg_listhdr g_cfg_dirlist = { 0 };
Cfg_listhdr g_cfg_partitionlist = { 0 };

int g_cfg_file_pathfield = 0;

void
config_init_menus(Cfg_menu *menuptr)
{
	void	*voidptr;
	const char *name_str;
	Cfg_defval *defptr;
	int	type;
	int	pos;
	int	val;

	if(menuptr[0].defptr != 0) {
		return;
	}
	menuptr[0].defptr = (void *)1;
	pos = 0;
	while(pos < 100) {
		type = menuptr->cfgtype;
		voidptr = menuptr->ptr;
		name_str = menuptr->name_str;
		if(menuptr->str == 0) {
			break;
		}
		if(name_str != 0) {
			defptr = &(g_cfg_defvals[g_cfg_defval_index++]);
			if(g_cfg_defval_index >= CFG_MAX_DEFVALS) {
				printf("CFG_MAX_DEFVAL overflow\n");
				my_exit(5);
			}
			defptr->menuptr = menuptr;
			defptr->intval = 0;
			switch(type) {
			case CFGTYPE_INT:
				val = *((int *)voidptr);
				defptr->intval = val;
				menuptr->defptr = &(defptr->intval);
				break;
			default:
				printf("name_str is %p = %s, but type: %d\n",
					name_str, name_str, type);
				my_exit(5);
			}
		}
		if(type == CFGTYPE_MENU) {
			config_init_menus((Cfg_menu *)voidptr);
		}
		pos++;
		menuptr++;
	}
}

void
config_init()
{
	config_init_menus(g_cfg_main_menu);

	// Find the config.kegs file
	setup_kegs_file(&g_config_kegs_name[0], sizeof(g_config_kegs_name), 0,
					&g_config_kegs_name_list[0]);

	config_parse_config_kegs_file();
}

void
cfg_exit()
{
	/* printf("In cfg exit\n"); */
	g_config_control_panel = 0;
}

void
cfg_text_screen_dump()
{
	char	buf[85];
	char	*filename;
	FILE	*ofile;
	int	offset;
	int	c;
	int	pos;
	int	i, j;

	filename = "kegs.screen.dump";
	printf("Writing text screen to the file %s\n", filename);
	ofile = fopen(filename, "w");
	if(ofile == 0) {
		printf("fopen ret 0, errno: %d\n", errno);
		return;
	}

	for(i = 0; i < 24; i++) {
		pos = 0;
		for(j = 0; j < 40; j++) {
			offset = g_screen_index[i] + j;
			if(g_save_cur_a2_stat & ALL_STAT_VID80) {
				c = g_save_text_screen_bytes[0x400+offset];
				c = c & 0x7f;
				if(c < 0x20) {
					c += 0x40;
				}
				buf[pos++] = c;
			}
			c = g_save_text_screen_bytes[offset] & 0x7f;
			if(c < 0x20) {
				c += 0x40;
			}
			buf[pos++] = c;
		}
		while((pos > 0) && (buf[pos-1] == ' ')) {
			/* try to strip out trailing spaces */
			pos--;
		}
		buf[pos++] = '\n';
		buf[pos++] = 0;
		fputs(buf, ofile);
	}
	fclose(ofile);
}

void
config_vbl_update(int doit_3_persec)
{
	if(doit_3_persec) {
		if(g_config_kegs_auto_update && g_config_kegs_update_needed) {
			config_write_config_kegs_file();
		}
	}
	return;
}

void
config_parse_option(char *buf, int pos, int len, int line)
{
	Cfg_menu *menuptr;
	Cfg_defval *defptr;
	char	*nameptr;
	int	*iptr;
	int	num_equals;
	int	type;
	int	val;
	int	c;
	int	i;

// warning: modifies buf (turns spaces to nulls)
// parse buf from pos into option, "=" and then "rest"
	if(pos >= len) {
		/* blank line */
		return;
	}

	if(strncmp(&buf[pos], "bram", 4) == 0) {
		config_parse_bram(buf, pos+4, len);
		return;
	}

	// find "name" as first contiguous string
	printf("...parse_option: line %d, %p,%p = %s (%s) len:%d\n", line,
		&buf[pos], buf, &buf[pos], buf, len);

	nameptr = &buf[pos];
	while(pos < len) {
		c = buf[pos];
		if(c == 0 || c == ' ' || c == '\t' || c == '\n') {
			break;
		}
		pos++;
	}
	buf[pos] = 0;
	pos++;

	// Eat up all whitespace and '='
	num_equals = 0;
	while(pos < len) {
		c = buf[pos];
		if((c == '=') && num_equals == 0) {
			pos++;
			num_equals++;
		} else if(c == ' ' || c == '\t') {
			pos++;
		} else {
			break;
		}
	}

	/* Look up nameptr to find type */
	type = -1;
	defptr = 0;
	menuptr = 0;
	for(i = 0; i < g_cfg_defval_index; i++) {
		defptr = &(g_cfg_defvals[i]);
		menuptr = defptr->menuptr;
		if(strcmp(menuptr->name_str, nameptr) == 0) {
			type = menuptr->cfgtype;
			break;
		}
	}

	switch(type) {
	case CFGTYPE_INT:
		/* use strtol */
		val = (int)strtol(&buf[pos], 0, 0);
		iptr = (int *)menuptr->ptr;
		*iptr = val;
		break;
	default:
		printf("Config file variable %s is unknown type: %d\n",
			nameptr, type);
	}

}

void
config_parse_bram(char *buf, int pos, int len)
{
	int	bram_num;
	int	offset;
	int	val;

	if((len < (pos+5)) || (buf[pos+1] != '[') || (buf[pos+4] != ']')) {
		printf("Malformed bram statement: %s\n", buf);
		return;
	}
	bram_num = buf[pos] - '0';
	if(bram_num != 1 && bram_num != 3) {
		printf("Malformed bram number: %s\n", buf);
		return;
	}

	bram_num = bram_num >> 1;	// turn 3->1 and 1->0

	offset = strtoul(&(buf[pos+2]), 0, 16);
	pos += 5;
	while(pos < len) {
		while(buf[pos] == ' ' || buf[pos] == '\t' || buf[pos] == 0x0a ||
				buf[pos] == 0x0d || buf[pos] == '=') {
			pos++;
		}
		val = strtoul(&buf[pos], 0, 16);
		clk_bram_set(bram_num, offset, val);
		offset++;
		pos += 2;
	}
}

void
config_parse_config_kegs_file()
{
	FILE	*fconf;
	char	*buf;
	char	*ptr;
	char	*name_ptr;
	char	*partition_name;
	int	part_num;
	int	ejected;
	int	line;
	int	pos;
	int	slot;
	int	drive;
	int	size;
	int	len;
	int	ret;
	int	i;

	printf("Parsing config.kegs file\n");

	clk_bram_zero();

	g_highest_smartport_unit = -1;

	cfg_get_base_path(&g_cfg_cwd_str[0], g_config_kegs_name, 0);
	if(g_cfg_cwd_str[0] != 0) {
		ret = chdir(&g_cfg_cwd_str[0]);
		if(ret != 0) {
			printf("chdir to %s, errno:%d\n", g_cfg_cwd_str, errno);
		}
	}

	/* In any case, copy the directory path to g_cfg_cwd_str */
	(void)getcwd(&g_cfg_cwd_str[0], CFG_PATH_MAX);

	fconf = fopen(g_config_kegs_name, "rt");
	if(fconf == 0) {
		printf("cannot open disk_conf!  Stopping!\n");
		exit(3);
	}

	line = 0;
	while(1) {
		buf = &g_config_kegs_buf[0];
		ptr = fgets(buf, CONF_BUF_LEN, fconf);
		if(ptr == 0) {
			iwm_printf("Done reading disk_conf\n");
			break;
		}

		line++;
		/* strip off newline(s) */
		len = strlen(buf);
		for(i = len - 1; i >= 0; i--) {
			if((buf[i] != 0x0d) && (buf[i] != 0x0a)) {
				break;
			}
			len = i;
			buf[i] = 0;
		}

		iwm_printf("disk_conf[%d]: %s\n", line, buf);
		if(len > 0 && buf[0] == '#') {
			iwm_printf("Skipping comment\n");
			continue;
		}

		/* determine what this is */
		pos = 0;

		while(pos < len && (buf[pos] == ' ' || buf[pos] == '\t') ) {
			pos++;
		}
		if((pos + 4) > len || buf[pos] != 's' || buf[pos+2] != 'd' ||
				buf[pos+1] > '9' || buf[pos+1] < '0') {
			config_parse_option(buf, pos, len, line);
			continue;
		}

		slot = buf[pos+1] - '0';
		drive = buf[pos+3] - '0';

		/* skip over slot, drive */
		pos += 4;
		if(buf[pos] >= '0' && buf[pos] <= '9') {
			drive = drive * 10 + buf[pos] - '0';
			pos++;
		}

		/*	make s6d1 mean index 0 */
		drive--;

		while(pos < len && (buf[pos] == ' ' || buf[pos] == '\t' ||
					buf[pos] == '=') ) {
			pos++;
		}

		ejected = 0;
		if(buf[pos] == '#') {
			/* disk is ejected, but read all the info anyway */
			ejected = 1;
			pos++;
		}

		size = 0;
		if(buf[pos] == ',') {
			/* read optional size parameter */
			pos++;
			while(pos < len && buf[pos] >= '0' && buf[pos] <= '9'){
				size = size * 10 + buf[pos] - '0';
				pos++;
			}
			size = size * 1024;
			if(buf[pos] == ',') {
				pos++;		/* eat trailing ',' */
			}
		}

		/* see if it has a partition name */
		partition_name = 0;
		part_num = -1;
		if(buf[pos] == ':') {
			pos++;
			/* yup, it's got a partition name! */
			partition_name = &buf[pos];
			while((pos < len) && (buf[pos] != ':')) {
				pos++;
			}
			buf[pos] = 0;	/* null terminate partition name */
			pos++;
		}
		if(buf[pos] == ';') {
			pos++;
			/* it's got a partition number */
			part_num = 0;
			while((pos < len) && (buf[pos] != ':')) {
				part_num = (10*part_num) + buf[pos] - '0';
				pos++;
			}
			pos++;
		}

		/* Get filename */
		name_ptr = &(buf[pos]);
		if(name_ptr[0] == 0) {
			continue;
		}

		insert_disk(slot, drive, name_ptr, ejected, size,
						partition_name, part_num);

	}

	ret = fclose(fconf);
	if(ret != 0) {
		printf("Closing disk_conf ret: %d, errno: %d\n", ret, errno);
		exit(4);
	}

	iwm_printf("Done parsing disk_conf file\n");
}


Disk *
cfg_get_dsk_from_slot_drive(int slot, int drive)
{
	Disk	*dsk;
	int	max_drive;

	/* Get dsk */
	max_drive = 2;
	switch(slot) {
	case 5:
		dsk = &(iwm.drive35[drive]);
		break;
	case 6:
		dsk = &(iwm.drive525[drive]);
		break;
	default:
		max_drive = MAX_C7_DISKS;
		dsk = &(iwm.smartport[drive]);
	}

	if(drive >= max_drive) {
		dsk -= drive;	/* move back to drive 0 effectively */
	}

	return dsk;
}

void
config_generate_config_kegs_name(char *outstr, int maxlen, Disk *dsk,
		int with_extras)
{
	char	*str;

	str = outstr;

	if(with_extras && dsk->fd < 0) {
		snprintf(str, maxlen - (str - outstr), "#");
		str = &outstr[strlen(outstr)];
	}
	if(with_extras && dsk->force_size > 0) {
		snprintf(str, maxlen - (str - outstr), ",%d,", dsk->force_size);
		str = &outstr[strlen(outstr)];
	}
	if(with_extras && dsk->partition_name != 0) {
		snprintf(str, maxlen - (str - outstr), ":%s:",
							dsk->partition_name);
		str = &outstr[strlen(outstr)];
	} else if(with_extras && dsk->partition_num >= 0) {
		snprintf(str, maxlen - (str - outstr), ";%d:",
							dsk->partition_num);
		str = &outstr[strlen(outstr)];
	}
	snprintf(str, maxlen - (str - outstr), "%s", dsk->name_ptr);
}

void
config_write_config_kegs_file()
{
	FILE	*fconf;
	Disk	*dsk;
	Cfg_defval *defptr;
	Cfg_menu *menuptr;
	int	defval, curval;
	int	type;
	int	slot, drive;
	int	i;

	printf("Writing config.kegs file to %s\n", g_config_kegs_name);

	fconf = fopen(g_config_kegs_name, "w+");
	if(fconf == 0) {
		halt_printf("cannot open %s!  Stopping!\n");
		return;
	}

	fprintf(fconf, "# KEGS configuration file version %s\n",
						g_kegs_version_str);

	for(i = 0; i < MAX_C7_DISKS + 4; i++) {
		slot = 7;
		drive = i - 4;
		if(i < 4) {
			slot = (i >> 1) + 5;
			drive = i & 1;
		}
		if(drive == 0) {
			fprintf(fconf, "\n");	/* an extra blank line */
		}

		dsk = cfg_get_dsk_from_slot_drive(slot, drive);
		if(dsk->name_ptr == 0 && (i > 4)) {
			/* No disk, not even ejected--just skip */
			continue;
		}
		fprintf(fconf, "s%dd%d = ", slot, drive + 1);
		if(dsk->name_ptr == 0) {
			fprintf(fconf, "\n");
			continue;
		}
		config_generate_config_kegs_name(&g_cfg_tmp_path[0],
							CFG_PATH_MAX, dsk, 1);
		fprintf(fconf, "%s\n", &g_cfg_tmp_path[0]);
	}

	fprintf(fconf, "\n");

	/* See if any variables are different than their default */
	for(i = 0; i < g_cfg_defval_index; i++) {
		defptr = &(g_cfg_defvals[i]);
		menuptr = defptr->menuptr;
		defval = defptr->intval;
		type = menuptr->cfgtype;
		if(type != CFGTYPE_INT) {
			/* skip it */
			continue;
		}
		curval = *((int *)menuptr->ptr);
		if(curval != defval) {
			fprintf(fconf, "%s = %d\n", menuptr->name_str, curval);
		}
	}

	fprintf(fconf, "\n");

	/* write bram state */
	clk_write_bram(fconf);

	fclose(fconf);

	g_config_kegs_update_needed = 0;
}

void
insert_disk(int slot, int drive, const char *name, int ejected, int force_size,
		const char *partition_name, int part_num)
{
	byte	buf_2img[512];
	Disk	*dsk;
	char	*name_ptr, *uncomp_ptr, *system_str;
	char	*part_ptr;
	int	size;
	int	system_len;
	int	part_len;
	int	cmp_o, cmp_p, cmp_dot;
	int	cmp_b, cmp_i, cmp_n;
	int	can_write;
	int	len;
	int	nibs;
	int	unix_pos;
	int	name_len;
	int	image_identified;
	int	exp_size;
	int	save_track;
	int	ret;
	int	tmp;
	int	i;

	g_config_kegs_update_needed = 1;

	if((slot < 5) || (slot > 7)) {
		printf("insert_disk: Invalid slot: %d\n", slot);
		return;
	}
	if(drive < 0 || ((slot == 7) && (drive >= MAX_C7_DISKS)) ||
					((slot < 7) && (drive > 1))) {
		printf("insert_disk: Invalid drive: %d\n", drive);
			return;
	}

	dsk = cfg_get_dsk_from_slot_drive(slot, drive);

#if 0
	printf("Inserting disk %s (%s or %d) in slot %d, drive: %d\n", name,
		partition_name, part_num, slot, drive);
#endif

	dsk->just_ejected = 0;
	dsk->force_size = force_size;

	if(dsk->fd >= 0) {
		eject_disk(dsk);
	}

	/* Before opening, make sure no other mounted disk has this name */
	/* If so, unmount it */
	if(!ejected) {
		for(i = 0; i < 2; i++) {
			eject_named_disk(&iwm.drive525[i], name,partition_name);
			eject_named_disk(&iwm.drive35[i], name, partition_name);
		}
		for(i = 0; i < MAX_C7_DISKS; i++) {
			eject_named_disk(&iwm.smartport[i],name,partition_name);
		}
	}

	if(dsk->name_ptr != 0) {
		/* free old name_ptr */
		free(dsk->name_ptr);
	}

	name_len = strlen(name) + 1;
	name_ptr = (char *)malloc(name_len);
	strncpy(name_ptr, name, name_len);
	dsk->name_ptr = name_ptr;

	dsk->partition_name = 0;
	if(partition_name != 0) {
		part_len = strlen(partition_name) + 1;
		part_ptr = (char *)malloc(part_len);
		strncpy(part_ptr, partition_name, part_len);
		dsk->partition_name = part_ptr;
	}
	dsk->partition_num = part_num;

	iwm_printf("Opening up disk image named: %s\n", name_ptr);

	if(ejected) {
		/* just get out of here */
		dsk->fd = -1;
		return;
	}

	dsk->fd = -1;
	can_write = 1;

	if((name_len > 4) && (strcmp(&name_ptr[name_len - 4], ".gz") == 0)) {

		/* it's gzip'ed, try to gunzip it, then unlink the */
		/*   uncompressed file */

		can_write = 0;

		uncomp_ptr = (char *)malloc(name_len);
		strncpy(uncomp_ptr, name_ptr, name_len);
		uncomp_ptr[name_len - 4] = 0;

		system_len = name_len + 200;
		system_str = (char *)malloc(system_len + 1);
		snprintf(system_str, system_len,
			"set -o noclobber;gunzip -c %s > %s", name_ptr,
			uncomp_ptr);
		printf("I am uncompressing %s into %s for mounting\n",
							name_ptr, uncomp_ptr);
		ret = system(system_str);
		if(ret == 0) {
			/* successfully ran */
			dsk->fd = open(uncomp_ptr, O_RDONLY | O_BINARY, 0x1b6);
			iwm_printf("Opening .gz file %s is fd: %d\n",
							uncomp_ptr, dsk->fd);

			/* and, unlink the temporary file */
			(void)unlink(uncomp_ptr);
		}
		free(system_str);
		free(uncomp_ptr);
	}

	if(dsk->fd < 0 && can_write) {
		dsk->fd = open(name_ptr, O_RDWR | O_BINARY, 0x1b6);
	}

	if(dsk->fd < 0 && can_write) {
		printf("Trying to open %s read-only, errno: %d\n", name_ptr,
								errno);
		dsk->fd = open(name_ptr, O_RDONLY | O_BINARY, 0x1b6);
		can_write = 0;
	}

	iwm_printf("open returned: %d\n", dsk->fd);

	if(dsk->fd < 0) {
		printf("Disk image %s does not exist!\n", name_ptr);
		return;
	}

	if(can_write != 0) {
		dsk->write_prot = 0;
		dsk->write_through_to_unix = 1;
	} else {
		dsk->write_prot = 1;
		dsk->write_through_to_unix = 0;
	}

	save_track = dsk->cur_qtr_track;	/* save arm position */
	dsk->image_type = DSK_TYPE_PRODOS;

	/* See if it is in 2IMG format */
	ret = read(dsk->fd, (char *)&buf_2img[0], 512);
	size = force_size;
	if(size <= 0) {
		size = cfg_get_fd_size(dsk->fd);
	}
	image_identified = 0;
	if(buf_2img[0] == '2' && buf_2img[1] == 'I' && buf_2img[2] == 'M' &&
			buf_2img[3] == 'G') {
		/* It's a 2IMG disk */
		printf("Image named %s is in 2IMG format\n", dsk->name_ptr);
		image_identified = 1;

		if(buf_2img[12] == 0) {
			printf("2IMG is in DOS 3.3 sector order\n");
			dsk->image_type = DSK_TYPE_DOS33;
		}
		if(buf_2img[19] & 0x80) {
			/* disk is locked */
			printf("2IMG is write protected\n");
			dsk->write_prot = 1;
			dsk->write_through_to_unix = 0;
		}
		if((buf_2img[17] & 1) && (dsk->image_type == DSK_TYPE_DOS33)) {
			dsk->vol_num = buf_2img[16];
			printf("Setting DOS 3.3 vol num to %d\n", dsk->vol_num);
		}
		//	Some 2IMG archives have the size byte reversed
		size = (buf_2img[31] << 24) + (buf_2img[30] << 16) +
				(buf_2img[29] << 8) + buf_2img[28];
		unix_pos = (buf_2img[27] << 24) + (buf_2img[26] << 16) +
				(buf_2img[25] << 8) + buf_2img[24];
		if(size == 0x800c00) {
			//	Byte reversed 0x0c8000
			size = 0x0c8000;
		}
		if(size == 0) {
			/* From KEGS-OS-X: Gilles Tschopp: */
			/*  deal with corrupted 2IMG files */
			printf("Bernie corrupted size to 0...working around\n");

			/* Just get the full size, and subtract 64, and */
			/*  then round down to lower 0x1000 boundary */
			size = cfg_get_fd_size(dsk->fd) - 64;
			size = size & -0x1000;
		}
		dsk->image_start = unix_pos;
		dsk->image_size = size;
	}
	exp_size = 800*1024;
	if(dsk->disk_525) {
		exp_size = 140*1024;
	}
	if(!image_identified) {
		/* See if it might be the Mac diskcopy format */
		tmp = (buf_2img[0x40] << 24) + (buf_2img[0x41] << 16) +
				(buf_2img[0x42] << 8) + buf_2img[0x43];
		if((size >= (exp_size + 0x54)) && (tmp == exp_size)) {
			/* It's diskcopy since data size field matches */
			printf("Image named %s is in Mac diskcopy format\n",
								dsk->name_ptr);
			image_identified = 1;
			dsk->image_start = 0x54;
			dsk->image_size = exp_size;
			dsk->image_type = DSK_TYPE_PRODOS;	/* ProDOS */
		}
	}
	if(!image_identified) {
		/* Assume raw image */
		dsk->image_start = 0;
		dsk->image_size = size;
		dsk->image_type = DSK_TYPE_PRODOS;
		if(dsk->disk_525) {
			dsk->image_type = DSK_TYPE_DOS33;
			if(name_len >= 5) {
				cmp_o = dsk->name_ptr[name_len-2];
				cmp_p = dsk->name_ptr[name_len-3];
				cmp_dot = dsk->name_ptr[name_len-4];
				if(cmp_dot == '.' &&
					  (cmp_p == 'p' || cmp_p == 'P') &&
					  (cmp_o == 'o' || cmp_o == 'O')) {
					dsk->image_type = DSK_TYPE_PRODOS;
				}

				cmp_b = dsk->name_ptr[name_len-2];
				cmp_i = dsk->name_ptr[name_len-3];
				cmp_n = dsk->name_ptr[name_len-4];
				cmp_dot = dsk->name_ptr[name_len-5];
				if(cmp_dot == '.' &&
					  (cmp_n == 'n' || cmp_n == 'N') &&
					  (cmp_i == 'i' || cmp_i == 'I') &&
					  (cmp_b == 'b' || cmp_b == 'B')) {
					dsk->image_type = DSK_TYPE_NIB;
					dsk->write_prot = 1;
					dsk->write_through_to_unix = 0;
				}
			}
		}
	}

	dsk->disk_dirty = 0;
	dsk->nib_pos = 0;

	if(dsk->smartport) {
		g_highest_smartport_unit = MAX(dsk->drive,
						g_highest_smartport_unit);

		if(partition_name != 0 || part_num >= 0) {
			ret = cfg_partition_find_by_name_or_num(dsk->fd,
						partition_name, part_num, dsk);
			printf("partition %s (num %d) mounted, wr_prot: %d\n",
				partition_name, part_num, dsk->write_prot);

			if(ret < 0) {
				close(dsk->fd);
				dsk->fd = -1;
				return;
			}
		}
		iwm_printf("adding smartport device[%d], size:%08x, "
			"img_sz:%08x\n", dsk->drive, dsk->tracks[0].unix_len,
			dsk->image_size);
	} else if(dsk->disk_525) {
		unix_pos = dsk->image_start;
		size = dsk->image_size;
		dsk->num_tracks = 4*35;
		len = 0x1000;
		nibs = NIB_LEN_525;
		if(dsk->image_type == DSK_TYPE_NIB) {
			len = dsk->image_size / 35;;
			nibs = len;
		}
		if(size != 35*len) {
			printf("Disk 5.25 error: size is %d, not %d\n",size,
					35*len);
		}
		for(i = 0; i < 35; i++) {
			iwm_move_to_track(dsk, 4*i);
			disk_unix_to_nib(dsk, 4*i, unix_pos, len, nibs);
			unix_pos += len;
		}
	} else {
		/* disk_35 */
		unix_pos = dsk->image_start;
		size = dsk->image_size;
		if(size != 800*1024) {
			printf("Disk 3.5 error: size is %d, not 800K\n", size);
		}
		dsk->num_tracks = 2*80;
		for(i = 0; i < 2*80; i++) {
			iwm_move_to_track(dsk, i);
			len = g_track_bytes_35[i >> 5];
			nibs = g_track_nibs_35[i >> 5];
			iwm_printf("Trk: %d.%d = unix: %08x, %04x, %04x\n",
				i>>1, i & 1, unix_pos, len, nibs);
			disk_unix_to_nib(dsk, i, unix_pos, len, nibs);
			unix_pos += len;

			iwm_printf(" trk_len:%05x\n",dsk->tracks[i].track_len);
		}
	}

	iwm_move_to_track(dsk, save_track);

}

void
eject_named_disk(Disk *dsk, const char *name, const char *partition_name)
{

	if(dsk->fd < 0) {
		return;
	}

	/* If name matches, eject the disk! */
	if(!strcmp(dsk->name_ptr, name)) {
		/* It matches, eject it */
		if((partition_name != 0) && (dsk->partition_name != 0)) {
			/* If both have partitions, and they differ, then */
			/*  don't eject.  Otherwise, eject */
			if(strcmp(dsk->partition_name, partition_name) != 0) {
				/* Don't eject */
				return;
			}
		}
		eject_disk(dsk);
	}
}

void
eject_disk_by_num(int slot, int drive)
{
	Disk	*dsk;

	dsk = cfg_get_dsk_from_slot_drive(slot, drive);

	eject_disk(dsk);
}

void
eject_disk(Disk *dsk)
{
	int	motor_on;
	int	i;

	if(dsk->fd < 0) {
		return;
	}

	g_config_kegs_update_needed = 1;

	motor_on = iwm.motor_on;
	if(g_apple35_sel) {
		motor_on = iwm.motor_on35;
	}
	if(motor_on) {
		halt_printf("Try eject dsk:%s, but motor_on!\n", dsk->name_ptr);
	}

	iwm_flush_disk_to_unix(dsk);

	printf("Ejecting disk: %s\n", dsk->name_ptr);

	/* Free all memory, close file */

	/* free the tracks first */
	for(i = 0; i < dsk->num_tracks; i++) {
		if(dsk->tracks[i].nib_area) {
			free(dsk->tracks[i].nib_area);
		}
		dsk->tracks[i].nib_area = 0;
		dsk->tracks[i].track_len = 0;
	}
	dsk->num_tracks = 0;

	/* close file, clean up dsk struct */
	close(dsk->fd);

	dsk->image_start = 0;
	dsk->image_size = 0;
	dsk->nib_pos = 0;
	dsk->disk_dirty = 0;
	dsk->write_through_to_unix = 0;
	dsk->write_prot = 1;
	dsk->fd = -1;
	dsk->just_ejected = 1;

	/* Leave name_ptr valid */
}

int
cfg_get_fd_size(int fd)
{
	struct stat stat_buf;
	int	ret;

	ret = fstat(fd, &stat_buf);
	if(ret != 0) {
		fprintf(stderr,"fstat returned %d on fd %d, errno: %d\n",
			ret, fd, errno);
		stat_buf.st_size = 0;
	}

	return stat_buf.st_size;
}

int
cfg_partition_read_block(int fd, void *buf, int blk, int blk_size)
{
	int	ret;

	ret = lseek(fd, blk * blk_size, SEEK_SET);
	if(ret != blk * blk_size) {
		printf("lseek: %08x, wanted: %08x, errno: %d\n", ret,
			blk * blk_size, errno);
		return 0;
	}

	ret = read(fd, (char *)buf, blk_size);
	if(ret != blk_size) {
		printf("ret: %08x, wanted %08x, errno: %d\n", ret, blk_size,
			errno);
		return 0;
	}
	return ret;
}

int
cfg_partition_find_by_name_or_num(int fd, const char *partnamestr, int part_num,
		Disk *dsk)
{
	Cfg_dirent *direntptr;
	int	match;
	int	num_parts;
	int	i;

	num_parts = cfg_partition_make_list(fd);

	if(num_parts <= 0) {
		return -1;
	}

	for(i = 0; i < g_cfg_partitionlist.last; i++) {
		direntptr = &(g_cfg_partitionlist.direntptr[i]);
		match = 0;
		if((strncmp(partnamestr, direntptr->name, 32) == 0) &&
							(part_num < 0)) {
			//printf("partition, match1, name:%s %s, part_num:%d\n",
			//	partnamestr, direntptr->name, part_num);

			match = 1;
		}
		if((partnamestr == 0) && (direntptr->part_num == part_num)) {
			//printf("partition, match2, n:%s, part_num:%d == %d\n",
			//	direntptr->name, direntptr->part_num, part_num);
			match = 1;
		}
		if(match) {
			dsk->image_start = direntptr->image_start;
			dsk->image_size = direntptr->size;
			//printf("match with image_start: %08x, image_size: "
			//	"%08x\n", dsk->image_start, dsk->image_size);

			return i;
		}
	}

	return -1;
}

int
cfg_partition_make_list(int fd)
{
	Driver_desc *driver_desc_ptr;
	Part_map *part_map_ptr;
	word32	*blk_bufptr;
	word32	start;
	word32	len;
	word32	data_off;
	word32	data_len;
	word32	sig;
	int	size;
	int	image_start, image_size;
	int	is_dir;
	int	block_size;
	int	map_blks;
	int	cur_blk;

	block_size = 512;

	cfg_free_alldirents(&g_cfg_partitionlist);

	blk_bufptr = (word32 *)malloc(MAX_PARTITION_BLK_SIZE);

	cfg_partition_read_block(fd, blk_bufptr, 0, block_size);

	driver_desc_ptr = (Driver_desc *)blk_bufptr;
	sig = GET_BE_WORD16(driver_desc_ptr->sig);
	block_size = GET_BE_WORD16(driver_desc_ptr->blk_size);
	if(block_size == 0) {
		block_size = 512;
	}
	if(sig != 0x4552 || block_size < 0x200 ||
				(block_size > MAX_PARTITION_BLK_SIZE)) {
		cfg_printf("Partition error: No driver descriptor map found\n");
		free(blk_bufptr);
		return 0;
	}

	map_blks = 1;
	cur_blk = 0;
	size = cfg_get_fd_size(fd);
	cfg_file_add_dirent(&g_cfg_partitionlist, "None - Whole image",
			is_dir=0, size, 0, -1);

	while(cur_blk < map_blks) {
		cur_blk++;
		cfg_partition_read_block(fd, blk_bufptr, cur_blk, block_size);
		part_map_ptr = (Part_map *)blk_bufptr;
		sig = GET_BE_WORD16(part_map_ptr->sig);
		if(cur_blk <= 1) {
			map_blks = MIN(20,
				GET_BE_WORD32(part_map_ptr->map_blk_cnt));
		}
		if(sig != 0x504d) {
			printf("Partition entry %d bad sig\n", cur_blk);
			free(blk_bufptr);
			return g_cfg_partitionlist.last;
		}

		/* found it, check for consistency */
		start = GET_BE_WORD32(part_map_ptr->phys_part_start);
		len = GET_BE_WORD32(part_map_ptr->part_blk_cnt);
		data_off = GET_BE_WORD32(part_map_ptr->data_start);
		data_len = GET_BE_WORD32(part_map_ptr->data_cnt);
		if(data_off + data_len > len) {
			printf("Poorly formed entry\n");
			continue;
		}

		if(data_len < 10 || start < 1) {
			printf("Poorly formed entry %d, datalen:%d, "
				"start:%08x\n", cur_blk, data_len, start);
			continue;
		}

		image_size = data_len * block_size;
		image_start = (start + data_off) * block_size;
		is_dir = 2*(image_size < 800*1024);
#if 0
		printf(" partition add entry %d = %s %d %08x %08x\n",
			cur_blk, part_map_ptr->part_name, is_dir,
			image_size, image_start);
#endif

		cfg_file_add_dirent(&g_cfg_partitionlist,
			part_map_ptr->part_name, is_dir, image_size,
			image_start, cur_blk);
	}

	free(blk_bufptr);
	return g_cfg_partitionlist.last;
}

int
cfg_maybe_insert_disk(int slot, int drive, const char *namestr)
{
	int	num_parts;
	int	fd;

	fd = open(namestr, O_RDONLY | O_BINARY, 0x1b6);
	if(fd < 0) {
		printf("Cannot open: %s\n", namestr);
		return 0;
	}

	num_parts = cfg_partition_make_list(fd);
	close(fd);

	if(num_parts > 0) {
		printf("Choose a partition\n");
		g_cfg_select_partition = 1;
	} else {
		insert_disk(slot, drive, namestr, 0, 0, 0, -1);
		return 1;
	}
	return 0;
}

int
cfg_stat(char *path, struct stat *sb)
{
	int	removed_slash;
	int	len;
	int	ret;

	removed_slash = 0;
	len = 0;

#ifdef _WIN32
	/* Windows doesn't like to stat paths ending in a /, so remove it */
	len = strlen(path);
	if((len > 1) && (path[len - 1] == '/') ) {
		path[len - 1] = 0;	/* remove the slash */
		removed_slash = 1;
	}
#endif

	ret = stat(path, sb);

#ifdef _WIN32
	/* put the slash back */
	if(removed_slash) {
		path[len - 1] = '/';
	}
#endif

	return ret;
}

void
cfg_htab_vtab(int x, int y)
{
	if(x > 79) {
		x = 0;
	}
	if(y > 23) {
		y = 0;
	}
	g_cfg_curs_x = x;
	g_cfg_curs_y = y;
	g_cfg_curs_inv = 0;
	g_cfg_curs_mousetext = 0;
}

void
cfg_home()
{
	int	i;

	cfg_htab_vtab(0, 0);
	for(i = 0; i < 24; i++) {
		cfg_cleol();
	}
}

void
cfg_cleol()
{
	g_cfg_curs_inv = 0;
	g_cfg_curs_mousetext = 0;
	cfg_putchar(' ');
	while(g_cfg_curs_x != 0) {
		cfg_putchar(' ');
	}
}

void
cfg_putchar(int c)
{
	int	offset;
	int	x, y;

	if(c == '\n') {
		cfg_cleol();
		return;
	}
	if(c == '\b') {
		g_cfg_curs_inv = !g_cfg_curs_inv;
		return;
	}
	if(c == '\t') {
		g_cfg_curs_mousetext = !g_cfg_curs_mousetext;
		return;
	}
	y = g_cfg_curs_y;
	x = g_cfg_curs_x;

	offset = g_screen_index[g_cfg_curs_y];
	if((x & 1) == 0) {
		offset += 0x10000;
	}
	if(g_cfg_curs_inv) {
		if(c >= 0x40 && c < 0x60) {
			c = c & 0x1f;
		}
	} else {
		c = c | 0x80;
	}
	if(g_cfg_curs_mousetext) {
		c = (c & 0x1f) | 0x40;
	}
	set_memory_c(0xe00400 + offset + (x >> 1), c, 0);
	x++;
	if(x >= 80) {
		x = 0;
		y++;
		if(y >= 24) {
			y = 0;
		}
	}
	g_cfg_curs_y = y;
	g_cfg_curs_x = x;
}

void
cfg_printf(const char *fmt, ...)
{
	va_list ap;
	int	c;
	int	i;

	va_start(ap, fmt);
	(void)vsnprintf(g_cfg_printf_buf, CFG_PRINTF_BUFSIZE, fmt, ap);
	va_end(ap);

	for(i = 0; i < CFG_PRINTF_BUFSIZE; i++) {
		c = g_cfg_printf_buf[i];
		if(c == 0) {
			return;
		}
		cfg_putchar(c);
	}
}

void
cfg_print_num(int num, int max_len)
{
	char	buf[64];
	char	buf2[64];
	int	len;
	int	cnt;
	int	c;
	int	i, j;

	/* Prints right-adjusted "num" in field "max_len" wide */
	snprintf(&buf[0], 64, "%d", num);
	len = strlen(buf);
	for(i = 0; i < 64; i++) {
		buf2[i] = ' ';
	}
	j = max_len + 1;
	buf2[j] = 0;
	j--;
	cnt = 0;
	for(i = len - 1; (i >= 0) && (j >= 1); i--) {
		c = buf[i];
		if(c >= '0' && c <= '9') {
			if(cnt >= 3) {
				buf2[j--] = ',';
				cnt = 0;
			}
			cnt++;
		}
		buf2[j--] = c;
	}
	cfg_printf(&buf2[1]);
}

void
cfg_get_disk_name(char *outstr, int maxlen, int type_ext, int with_extras)
{
	Disk	*dsk;
	int	slot, drive;

	slot = type_ext >> 8;
	drive = type_ext & 0xff;
	dsk = cfg_get_dsk_from_slot_drive(slot, drive);

	outstr[0] = 0;
	if(dsk->name_ptr == 0) {
		return;
	}

	config_generate_config_kegs_name(outstr, maxlen, dsk, with_extras);
}

void
cfg_parse_menu(Cfg_menu *menu_ptr, int menu_pos, int highlight_pos, int change)
{
	char	valbuf[CFG_OPT_MAXSTR];
	const char *menustr;
	char	*str;
	char	*outstr;
	int	*iptr;
	int	val;
	int	num_opts;
	int	opt_num;
	int	bufpos, outpos;
	int	curval, defval;
	int	type;
	int	type_ext;
	int	opt_get_str;
	int	separator;
	int	len;
	int	c;
	int	i;

	g_cfg_opt_buf[0] = 0;

	num_opts = 0;
	opt_get_str = 0;
	separator = ',';

	menu_ptr += menu_pos;		/* move forward to entry menu_pos */

	menustr = menu_ptr->str;
	type = menu_ptr->cfgtype;
	type_ext = (type >> 4);
	type = type & 0xf;
	len = strlen(menustr) + 1;

	bufpos = 0;
	outstr = &(g_cfg_opt_buf[0]);

	outstr[bufpos++] = ' ';
	outstr[bufpos++] = ' ';
	outstr[bufpos++] = '\t';
	outstr[bufpos++] = '\t';
	outstr[bufpos++] = ' ';
	outstr[bufpos++] = ' ';

	if(menu_pos == highlight_pos) {
		outstr[bufpos++] = '\b';
	}

	opt_get_str = 2;
	i = -1;
	outpos = bufpos;
#if 0
	printf("cfg menu_pos: %d str len: %d\n", menu_pos, len);
#endif
	while(++i < len) {
		c = menustr[i];
		if(c == separator) {
			if(i == 0) {
				continue;
			}
			c = 0;
		}
		outstr[outpos++] = c;
		outstr[outpos] = 0;
		if(outpos >= CFG_OPT_MAXSTR) {
			fprintf(stderr, "CFG_OPT_MAXSTR exceeded\n");
			my_exit(1);
		}
		if(c == 0) {
			if(opt_get_str == 2) {
				outstr = &(valbuf[0]);
				bufpos = outpos - 1;
				opt_get_str = 0;
			} else if(opt_get_str) {
#if 0
				if(menu_pos == highlight_pos) {
					printf("menu_pos %d opt %d = %s=%d\n",
						menu_pos, num_opts,
						g_cfg_opts_strs[num_opts],
						g_cfg_opts_vals[num_opts]);
				}
#endif
				num_opts++;
				outstr = &(valbuf[0]);
				opt_get_str = 0;
				if(num_opts >= CFG_MAX_OPTS) {
					fprintf(stderr, "CFG_MAX_OPTS oflow\n");
					my_exit(1);
				}
			} else {
				val = strtoul(valbuf, 0, 0);
				g_cfg_opts_vals[num_opts] = val;
				outstr = &(g_cfg_opts_strs[num_opts][0]);
				opt_get_str = 1;
			}
			outpos = 0;
			outstr[0] = 0;
		}
	}

	if(menu_pos == highlight_pos) {
		g_cfg_opt_buf[bufpos++] = '\b';
	}

	g_cfg_opt_buf[bufpos] = 0;

	// Figure out if we should get a checkmark
	curval = -1;
	defval = -1;
	if(type == CFGTYPE_INT) {
		iptr = menu_ptr->ptr;
		curval = *iptr;
		iptr = menu_ptr->defptr;
		defval = *iptr;
		if(curval == defval) {
			g_cfg_opt_buf[3] = 'D';	/* checkmark */
			g_cfg_opt_buf[4] = '\t';
		}
	}

	// Decide what to display on the "right" side
	str = 0;
	opt_num = -1;
	if(type == CFGTYPE_INT) {
		g_cfg_opt_buf[bufpos++] = ' ';
		g_cfg_opt_buf[bufpos++] = '=';
		g_cfg_opt_buf[bufpos++] = ' ';
		g_cfg_opt_buf[bufpos] = 0;
		for(i = 0; i < num_opts; i++) {
			if(curval == g_cfg_opts_vals[i]) {
				opt_num = i;
				break;
			}
		}
	}

	if(change != 0) {
		if(type == CFGTYPE_INT) {
			if(num_opts > 0) {
				opt_num += change;
				if(opt_num >= num_opts) {
					opt_num = 0;
				}
				if(opt_num < 0) {
					opt_num = num_opts - 1;
				}
				curval = g_cfg_opts_vals[opt_num];
			} else {
				curval += change;
				/* HACK: min_val, max_val testing here */
			}
			iptr = (int *)menu_ptr->ptr;
			*iptr = curval;
		}
		g_config_kegs_update_needed = 1;
	}

#if 0
	if(menu_pos == highlight_pos) {
		printf("menu_pos %d opt_num %d\n", menu_pos, opt_num);
	}
#endif

	if(opt_num >= 0) {
		str = &(g_cfg_opts_strs[opt_num][0]);
	} else {
		if(type == CFGTYPE_INT) {
			str = &(g_cfg_opts_strs[0][0]);
			snprintf(str, CFG_OPT_MAXSTR, "%d", curval);
		} else if (type == CFGTYPE_DISK) {
			str = &(g_cfg_opts_strs[0][0]),
			cfg_get_disk_name(str, CFG_OPT_MAXSTR, type_ext, 1);
			str = cfg_shorten_filename(str, 70);
		} else {
			str = "";
		}
	}

#if 0
	if(menu_pos == highlight_pos) {
		printf("menu_pos %d buf_pos %d, str is %s, %02x, %02x, "
			"%02x %02x\n",
			menu_pos, bufpos, str, g_cfg_opt_buf[bufpos-1],
			g_cfg_opt_buf[bufpos-2],
			g_cfg_opt_buf[bufpos-3],
			g_cfg_opt_buf[bufpos-4]);
	}
#endif

	g_cfg_opt_buf[bufpos] = 0;
	strncpy(&(g_cfg_opt_buf[bufpos]), str, CFG_OPT_MAXSTR - bufpos - 1);
	g_cfg_opt_buf[CFG_OPT_MAXSTR-1] = 0;
}

void
cfg_get_base_path(char *pathptr, const char *inptr, int go_up)
{
	const char *tmpptr;
	char	*slashptr;
	char	*outptr;
	int	add_dotdot, is_dotdot;
	int	len;
	int	c;

	/* Take full filename, copy it to pathptr, and truncate at last slash */
	/* inptr and pathptr can be the same */
	/* if go_up is set, then replace a blank dir with ".." */
	/* but first, see if path is currently just ../ over and over */
	/* if so, just tack .. onto the end and return */
	//printf("cfg_get_base start with %s\n", inptr);

	g_cfg_file_match[0] = 0;
	tmpptr = inptr;
	is_dotdot = 1;
	while(1) {
		if(tmpptr[0] == 0) {
			break;
		}
		if(tmpptr[0] == '.' && tmpptr[1] == '.' && tmpptr[2] == '/') {
			tmpptr += 3;
		} else {
			is_dotdot = 0;
			break;
		}
	}
	slashptr = 0;
	outptr = pathptr;
	c = -1;
	while(c != 0) {
		c = *inptr++;
		if(c == '/') {
			if(*inptr != 0) {	/* if not a trailing slash... */
				slashptr = outptr;
			}
		}
		*outptr++ = c;
	}
	if(!go_up) {
		/* if not go_up, copy chopped part to g_cfg_file_match*/
		/* copy from slashptr+1 to end */
		tmpptr = slashptr+1;
		if(slashptr == 0) {
			tmpptr = pathptr;
		}
		strncpy(&g_cfg_file_match[0], tmpptr, CFG_PATH_MAX);
		/* remove trailing / from g_cfg_file_match */
		len = strlen(&g_cfg_file_match[0]);
		if((len > 1) && (len < (CFG_PATH_MAX - 1)) &&
					g_cfg_file_match[len - 1] == '/') {
			g_cfg_file_match[len - 1] = 0;
		}
		//printf("set g_cfg_file_match to %s\n", &g_cfg_file_match[0]);
	}
	if(!is_dotdot && (slashptr != 0)) {
		slashptr[0] = '/';
		slashptr[1] = 0;
		outptr = slashptr + 2;
	}
	add_dotdot = 0;
	if(pathptr[0] == 0 || is_dotdot) {
		/* path was blank, or consisted of just ../ pattern */
		if(go_up) {
			add_dotdot = 1;
		}
	} else if(slashptr == 0) {
		/* no slashes found, but path was not blank--make it blank */
		if(pathptr[0] == '/') {
			pathptr[1] = 0;
		} else {
			pathptr[0] = 0;
		}
	}

	if(add_dotdot) {
		--outptr;
		outptr[0] = '.';
		outptr[1] = '.';
		outptr[2] = '/';
		outptr[3] = 0;
	}

	//printf("cfg_get_base end with %s, is_dotdot:%d, add_dotdot:%d\n",
	//		pathptr, is_dotdot, add_dotdot);
}

void
cfg_file_init()
{
	int	slot, drive;
	int	i;

	cfg_get_disk_name(&g_cfg_tmp_path[0], CFG_PATH_MAX, g_cfg_slotdrive, 0);

	slot = g_cfg_slotdrive >> 8;
	drive = g_cfg_slotdrive & 1;
	for(i = 0; i < 6; i++) {
		if(g_cfg_tmp_path[0] != 0) {
			break;
		}
		/* try to get a starting path from some mounted drive */
		drive = !drive;
		if(i & 1) {
			slot++;
			if(slot >= 8) {
				slot = 5;
			}
		}
		cfg_get_disk_name(&g_cfg_tmp_path[0], CFG_PATH_MAX,
							(slot << 8) + drive, 0);
	}
	cfg_get_base_path(&g_cfg_file_curpath[0], &g_cfg_tmp_path[0], 0);
	g_cfg_dirlist.invalid = 1;
}

void
cfg_free_alldirents(Cfg_listhdr *listhdrptr)
{
	int	i;

	if(listhdrptr->max > 0) {
		// Free the old directory listing
		for(i = 0; i < listhdrptr->last; i++) {
			free(listhdrptr->direntptr[i].name);
		}
		free(listhdrptr->direntptr);
	}

	listhdrptr->direntptr = 0;
	listhdrptr->last = 0;
	listhdrptr->max = 0;
	listhdrptr->invalid = 0;

	listhdrptr->topent = 0;
	listhdrptr->curent = 0;
}


void
cfg_file_add_dirent(Cfg_listhdr *listhdrptr, const char *nameptr, int is_dir,
		int size, int image_start, int part_num)
{
	Cfg_dirent *direntptr;
	char	*ptr;
	int	inc_amt;
	int	namelen;

	namelen = strlen(nameptr);
	if(listhdrptr->last >= listhdrptr->max) {
		// realloc
		inc_amt = MAX(64, listhdrptr->max);
		inc_amt = MIN(inc_amt, 1024);
		listhdrptr->max += inc_amt;
		listhdrptr->direntptr = realloc(listhdrptr->direntptr,
					listhdrptr->max * sizeof(Cfg_dirent));
	}
	ptr = malloc(namelen+1+is_dir);
	strncpy(ptr, nameptr, namelen+1);
	if(is_dir) {
		strcat(ptr, "/");
	}
#if 0
	printf("...file entry %d is %s\n", g_cfg_dirlist.last, ptr);
#endif
	direntptr = &(listhdrptr->direntptr[listhdrptr->last]);
	direntptr->name = ptr;
	direntptr->is_dir = is_dir;
	direntptr->size = size;
	direntptr->image_start = image_start;
	direntptr->part_num = part_num;
	listhdrptr->last++;
}

int
cfg_dirent_sortfn(const void *obj1, const void *obj2)
{
	const Cfg_dirent *direntptr1, *direntptr2;

	/* Called by qsort to sort directory listings */
	direntptr1 = (const Cfg_dirent *)obj1;
	direntptr2 = (const Cfg_dirent *)obj2;
	return strcmp(direntptr1->name, direntptr2->name);
}

int
cfg_str_match(const char *str1, const char *str2, int len)
{
	const byte *bptr1, *bptr2;
	int	c, c2;
	int	i;

	/* basically, work like strcmp, except if str1 ends first, return 0 */

	bptr1 = (const byte *)str1;
	bptr2 = (const byte *)str2;
	for(i = 0; i < len; i++) {
		c = *bptr1++;
		c2 = *bptr2++;
		if(c == 0) {
			if(i > 0) {
				return 0;
			} else {
				return c - c2;
			}
		}
		if(c != c2) {
			return c - c2;
		}
	}

	return 0;
}

void
cfg_file_readdir(const char *pathptr)
{
	struct dirent *direntptr;
	struct stat stat_buf;
	DIR	*dirptr;
	mode_t	fmt;
	char	*str;
	const char *tmppathptr;
	int	ret;
	int	is_dir, is_gz;
	int	len;
	int	i;

	if(!strncmp(pathptr, &g_cfg_file_cachedpath[0], CFG_PATH_MAX) &&
			(g_cfg_dirlist.last > 0) && (g_cfg_dirlist.invalid==0)){
		return;
	}
	// No match, must read new directory

	// Free all dirents that were cached previously
	cfg_free_alldirents(&g_cfg_dirlist);

	strncpy(&g_cfg_file_cachedpath[0], pathptr, CFG_PATH_MAX);
	strncpy(&g_cfg_file_cachedreal[0], pathptr, CFG_PATH_MAX);

	str = &g_cfg_file_cachedreal[0];

	for(i = 0; i < 200; i++) {
		len = strlen(str);
		if(len <= 0) {
			break;
		} else if(len < CFG_PATH_MAX-2) {
			if(str[len-1] != '/') {
				// append / to make various routines work
				str[len] = '/';
				str[len+1] = 0;
			}
		}
		ret = cfg_stat(str, &stat_buf);
		is_dir = 0;
		if(ret == 0) {
			fmt = stat_buf.st_mode & S_IFMT;
			if(fmt == S_IFDIR) {
				/* it's a directory */
				is_dir = 1;
			}
		}
		if(is_dir) {
			break;
		} else {
			// user is entering more path, use base for display
			cfg_get_base_path(str, str, 0);
		}
	}

	tmppathptr = str;
	if(str[0] == 0) {
		tmppathptr = ".";
	}
	cfg_file_add_dirent(&g_cfg_dirlist, "..", 1, 0, -1, -1);

	dirptr = opendir(tmppathptr);
	if(dirptr == 0) {
		printf("Could not open %s as a directory\n", tmppathptr);
		return;
	}
	while(1) {
		direntptr = readdir(dirptr);
		if(direntptr == 0) {
			break;
		}
		if(!strcmp(".", direntptr->d_name)) {
			continue;
		}
		if(!strcmp("..", direntptr->d_name)) {
			continue;
		}
		/* Else, see if it is a directory or a file */
		snprintf(&g_cfg_tmp_path[0], CFG_PATH_MAX, "%s%s",
				&g_cfg_file_cachedreal[0], direntptr->d_name);
		ret = cfg_stat(&g_cfg_tmp_path[0], &stat_buf);
		len = strlen(g_cfg_tmp_path);
		is_dir = 0;
		is_gz = 0;
		if((len > 3) && (strcmp(&g_cfg_tmp_path[len - 3], ".gz") == 0)){
			is_gz = 1;
		}
		if(ret != 0) {
			printf("stat %s ret %d, errno:%d\n", &g_cfg_tmp_path[0],
						ret, errno);
			stat_buf.st_size = 0;
		} else {
			fmt = stat_buf.st_mode & S_IFMT;
			if(fmt == S_IFDIR) {
				/* it's a directory */
				is_dir = 1;
			} else if((fmt == S_IFREG) && (is_gz == 0) &&
					(stat_buf.st_size < 140*1024)) {
				/* skip it */
				continue;
			}
		}
		cfg_file_add_dirent(&g_cfg_dirlist, direntptr->d_name, is_dir,
					stat_buf.st_size, -1, -1);
	}

	/* then sort the results (Mac's HFS+ is sorted, but other FS won't be)*/
	qsort(&(g_cfg_dirlist.direntptr[0]), g_cfg_dirlist.last,
					sizeof(Cfg_dirent), cfg_dirent_sortfn);

	g_cfg_dirlist.curent = g_cfg_dirlist.last - 1;
	for(i = g_cfg_dirlist.last - 1; i >= 0; i--) {
		ret = cfg_str_match(&g_cfg_file_match[0],
				g_cfg_dirlist.direntptr[i].name, CFG_PATH_MAX);
		if(ret <= 0) {
			/* set cur ent to closest filename to the match name */
			g_cfg_dirlist.curent = i;
		}
	}
}

char *
cfg_shorten_filename(const char *in_ptr, int maxlen)
{
	char	*out_ptr;
	int	len;
	int	c;
	int	i;

	/* Warning: uses a static string, not reentrant! */

	out_ptr = &(g_cfg_file_shortened[0]);
	len = strlen(in_ptr);
	maxlen = MIN(len, maxlen);
	for(i = 0; i < maxlen; i++) {
		c = in_ptr[i] & 0x7f;
		if(c < 0x20) {
			c = '*';
		}
		out_ptr[i] = c;
	}
	out_ptr[maxlen] = 0;
	if(len > maxlen) {
		for(i = 0; i < (maxlen/2); i++) {
			c = in_ptr[len-i-1] & 0x7f;
			if(c < 0x20) {
				c = '*';
			}
			out_ptr[maxlen-i-1] = c;
		}
		out_ptr[(maxlen/2) - 1] = '.';
		out_ptr[maxlen/2] = '.';
		out_ptr[(maxlen/2) + 1] = '.';
	}

	return out_ptr;
}

void
cfg_fix_topent(Cfg_listhdr *listhdrptr)
{
	int	num_to_show;

	num_to_show = listhdrptr->num_to_show;

	/* Force curent and topent to make sense */
	if(listhdrptr->curent >= listhdrptr->last) {
		listhdrptr->curent = listhdrptr->last - 1;
	}
	if(listhdrptr->curent < 0) {
		listhdrptr->curent = 0;
	}
	if(abs(listhdrptr->curent - listhdrptr->topent) >= num_to_show) {
		listhdrptr->topent = listhdrptr->curent - (num_to_show/2);
	}
	if(listhdrptr->topent > listhdrptr->curent) {
		listhdrptr->topent = listhdrptr->curent - (num_to_show/2);
	}
	if(listhdrptr->topent < 0) {
		listhdrptr->topent = 0;
	}
}

void
cfg_file_draw()
{
	Cfg_listhdr *listhdrptr;
	Cfg_dirent *direntptr;
	char	*str, *fmt;
	int	num_to_show;
	int	yoffset;
	int	x, y;
	int	i;

	cfg_file_readdir(&g_cfg_file_curpath[0]);

	for(y = 0; y < 21; y++) {
		cfg_htab_vtab(0, y);
		cfg_printf("\tZ\t");
		for(x = 1; x < 72; x++) {
			cfg_htab_vtab(x, y);
			cfg_putchar(' ');
		}
		cfg_htab_vtab(79, y);
		cfg_printf("\t_\t");
	}

	cfg_htab_vtab(1, 0);
	cfg_putchar('\b');
	for(x = 1; x < 79; x++) {
		cfg_putchar(' ');
	}
	cfg_htab_vtab(30, 0);
	cfg_printf("\bSelect image for s%dd%d\b", (g_cfg_slotdrive >> 8),
				(g_cfg_slotdrive & 0xff) + 1);

	cfg_htab_vtab(2, 1);
	cfg_printf("Current KEGS directory: %-50s",
			cfg_shorten_filename(&g_cfg_cwd_str[0], 50));

	cfg_htab_vtab(2, 2);
	cfg_printf("config.kegs path: %-50s",
			cfg_shorten_filename(&g_config_kegs_name[0], 50));

	cfg_htab_vtab(2, 3);

	str = "";
	if(g_cfg_file_pathfield) {
		str = "\b \b";
	}
	cfg_printf("Path: %s%s",
			cfg_shorten_filename(&g_cfg_file_curpath[0], 64), str);

	cfg_htab_vtab(0, 4);
	cfg_printf(" \t");
	for(x = 1; x < 79; x++) {
		cfg_putchar('\\');
	}
	cfg_printf("\t ");


	/* Force curent and topent to make sense */
	listhdrptr = &g_cfg_dirlist;
	num_to_show = CFG_NUM_SHOWENTS;
	yoffset = 5;
	if(g_cfg_select_partition > 0) {
		listhdrptr = &g_cfg_partitionlist;
		num_to_show -= 2;
		cfg_htab_vtab(2, yoffset);
		cfg_printf("Select partition of %-50s\n",
			cfg_shorten_filename(&g_cfg_file_path[0], 50), str);
		yoffset += 2;
	}

	listhdrptr->num_to_show = num_to_show;
	cfg_fix_topent(listhdrptr);
	for(i = 0; i < num_to_show; i++) {
		y = i + yoffset;
		if(listhdrptr->last > (i + listhdrptr->topent)) {
			direntptr = &(listhdrptr->
					direntptr[i + listhdrptr->topent]);
			cfg_htab_vtab(2, y);
			if(direntptr->is_dir) {
				cfg_printf("\tXY\t ");
			} else {
				cfg_printf("   ");
			}
			if(direntptr->part_num >= 0) {
				cfg_printf("%3d: ", direntptr->part_num);
			}
			str = cfg_shorten_filename(direntptr->name, 45);
			fmt = "%-45s";
			if(i + listhdrptr->topent == listhdrptr->curent) {
				if(g_cfg_file_pathfield == 0) {
					fmt = "\b%-45s\b";
				} else {
					fmt = "%-44s\b \b";
				}
			}
			cfg_printf(fmt, str);
			if(!direntptr->is_dir) {
				cfg_print_num(direntptr->size, 13);
			}
		}
	}

	cfg_htab_vtab(1, 21);
	cfg_putchar('\t');
	for(x = 1; x < 79; x++) {
		cfg_putchar('L');
	}
	cfg_putchar('\t');

}

void
cfg_partition_selected()
{
	char	*str;
	const char *part_str;
	char	*part_str2;
	int	pos;
	int	part_num;

	pos = g_cfg_partitionlist.curent;
	str = g_cfg_partitionlist.direntptr[pos].name;
	part_num = -2;
	part_str = 0;
	if(str[0] == 0 || (str[0] >= '0' && str[0] <= '9')) {
		part_num = g_cfg_partitionlist.direntptr[pos].part_num;
	} else {
		part_str = str;
	}
	part_str2 = 0;
	if(part_str != 0) {
		part_str2 = (char *)malloc(strlen(part_str)+1);
		strcpy(part_str2, part_str);
	}

	insert_disk(g_cfg_slotdrive >> 8, g_cfg_slotdrive & 0xff,
			&(g_cfg_file_path[0]), 0, 0, part_str2, part_num);
	if(part_str2 != 0) {
		free(part_str2);
	}
	g_cfg_slotdrive = -1;
	g_cfg_select_partition = -1;
}

void
cfg_file_selected()
{
	struct stat stat_buf;
	char	*str;
	int	fmt;
	int	ret;

	if(g_cfg_select_partition > 0) {
		cfg_partition_selected();
		return;
	}

	if(g_cfg_file_pathfield == 0) {
		// in file section area of window
		str = g_cfg_dirlist.direntptr[g_cfg_dirlist.curent].name;
		if(!strcmp(str, "../")) {
			/* go up one directory */
			cfg_get_base_path(&g_cfg_file_curpath[0],
						&g_cfg_file_curpath[0], 1);
			return;
		}

		snprintf(&g_cfg_file_path[0], CFG_PATH_MAX, "%s%s",
					&g_cfg_file_cachedreal[0], str);
	} else {
		// just use cfg_file_curpath directly
		strncpy(&g_cfg_file_path[0], &g_cfg_file_curpath[0],
							CFG_PATH_MAX);
	}

	ret = cfg_stat(&g_cfg_file_path[0], &stat_buf);
	fmt = stat_buf.st_mode & S_IFMT;
	cfg_printf("Stat'ing %s, st_mode is: %08x\n", &g_cfg_file_path[0],
			(int)stat_buf.st_mode);

	if(ret != 0) {
		printf("stat %s returned %d, errno: %d\n", &g_cfg_file_path[0],
					ret, errno);
	} else {
		if(fmt == S_IFDIR) {
			/* it's a directory */
			strncpy(&g_cfg_file_curpath[0], &g_cfg_file_path[0],
						CFG_PATH_MAX);
		} else {
			/* select it */
			ret = cfg_maybe_insert_disk(g_cfg_slotdrive >> 8,
				g_cfg_slotdrive & 0xff, &g_cfg_file_path[0]);
			if(ret > 0) {
				g_cfg_slotdrive = -1;
			}
		}
	}
}

void
cfg_file_handle_key(int key)
{
	Cfg_listhdr *listhdrptr;
	int	len;

	if(g_cfg_file_pathfield) {
		if(key >= 0x20 && key < 0x7f) {
			len = strlen(&g_cfg_file_curpath[0]);
			if(len < CFG_PATH_MAX-4) {
				g_cfg_file_curpath[len] = key;
				g_cfg_file_curpath[len+1] = 0;
			}
			return;
		}
	}

	listhdrptr = &g_cfg_dirlist;
	if(g_cfg_select_partition > 0) {
		listhdrptr = &g_cfg_partitionlist;
	}
	if( (g_cfg_file_pathfield == 0) &&
		 ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) ) {
		/* jump to file starting with this letter */
		g_cfg_file_match[0] = key;
		g_cfg_file_match[1] = 0;
		g_cfg_dirlist.invalid = 1;	/* re-read directory */
	}

	switch(key) {
	case 0x1b:
		eject_disk_by_num(g_cfg_slotdrive >> 8, g_cfg_slotdrive & 0xff);
		g_cfg_slotdrive = -1;
		g_cfg_select_partition = -1;
		g_cfg_dirlist.invalid = 1;
		break;
	case 0x0a:	/* down arrow */
		if(g_cfg_file_pathfield == 0) {
			listhdrptr->curent++;
			cfg_fix_topent(listhdrptr);
		}
		break;
	case 0x0b:	/* up arrow */
		if(g_cfg_file_pathfield == 0) {
			listhdrptr->curent--;
			cfg_fix_topent(listhdrptr);
		}
		break;
	case 0x0d:	/* return */
		cfg_file_selected();
		break;
	case 0x09:	/* tab */
		g_cfg_file_pathfield = !g_cfg_file_pathfield;
		break;
	case 0x08:	/* left arrow */
	case 0x7f:	/* delete key */
		if(g_cfg_file_pathfield) {
			// printf("left arrow/delete\n");
			len = strlen(&g_cfg_file_curpath[0]) - 1;
			if(len >= 0) {
				g_cfg_file_curpath[len] = 0;
			}
		}
		break;
	default:
		printf("key: %02x\n", key);
	}
#if 0
	printf("curent: %d, topent: %d, last: %d\n",
		g_cfg_dirlist.curent, g_cfg_dirlist.topent, g_cfg_dirlist.last);
#endif
}

void
config_control_panel()
{
	void	(*fn_ptr)();
	const char *str;
	Cfg_menu *menu_ptr;
	void	*ptr;
	int	print_eject_help;
	int	line;
	int	type;
	int	match_found;
	int	menu_line;
	int	menu_inc;
	int	max_line;
	int	key;
	int	i, j;

	// First, save key text info
	g_save_cur_a2_stat = g_cur_a2_stat;
	for(i = 0; i < 0x400; i++) {
		g_save_text_screen_bytes[i] = g_slow_memory_ptr[0x400+i];
		g_save_text_screen_bytes[0x400+i] =g_slow_memory_ptr[0x10400+i];
	}

	g_cur_a2_stat = ALL_STAT_TEXT | ALL_STAT_VID80 | ALL_STAT_ANNUNC3 |
			(0xf << BIT_ALL_STAT_TEXT_COLOR) | ALL_STAT_ALTCHARSET;
	g_a2_new_all_stat[0] = g_cur_a2_stat;
	g_new_a2_stat_cur_line = 0;

	cfg_printf("In config_control_panel\n");

	for(i = 0; i < 20; i++) {
		if(adb_read_c000() & 0x80) {
			(void)adb_access_c010();
		}
	}
	g_adb_repeat_vbl = 0;
	g_cfg_vbl_count = 0;
	// HACK: Force adb keyboard (and probably mouse) to "normal"...

	g_full_refresh_needed = -1;
	g_a2_screen_buffer_changed = -1;

	cfg_home();
	j = 0;

	menu_ptr = g_cfg_main_menu;
	menu_line = 1;
	menu_inc = 1;
	g_cfg_slotdrive = -1;
	g_cfg_select_partition = -1;

	while(g_config_control_panel) {
		cfg_home();
		line = 1;
		max_line = 1;
		match_found = 0;
		print_eject_help = 0;
		cfg_printf("%s\n\n", menu_ptr[0].str);
		while(line < 24) {
			str = menu_ptr[line].str;
			type = menu_ptr[line].cfgtype;
			ptr = menu_ptr[line].ptr;
			if(str == 0) {
				break;
			}
			if((type & 0xf) == CFGTYPE_DISK) {
				print_eject_help = 1;
			}
			cfg_parse_menu(menu_ptr, line, menu_line, 0);
			if(line == menu_line) {
				if(type != 0) {
					match_found = 1;
				} else if(menu_inc) {
					menu_line++;
				} else {
					menu_line--;
				}
			}
			if(line > max_line) {
				max_line = line;
			}

			cfg_printf("%s\n", g_cfg_opt_buf);
			line++;
		}
		if((menu_line < 1) && !match_found) {
			menu_line = 1;
		}
		if((menu_line >= max_line) && !match_found) {
			menu_line = max_line;
		}

		cfg_htab_vtab(0, 23);
		cfg_printf("Move: \tJ\t \tK\t Change: \tH\t \tU\t \tM\t");
		if(print_eject_help) {
			cfg_printf("   Eject: ");
			if(g_cfg_slotdrive >= 0) {
				cfg_printf("\bESC\b");
			} else {
				cfg_printf("E");
			}
		}
#if 0
		cfg_htab_vtab(0, 22);
		cfg_printf("menu_line: %d line: %d, vbl:%d, adb:%d key_dn:%d\n",
			menu_line, line, g_cfg_vbl_count, g_adb_repeat_vbl,
			g_key_down);
#endif

		if(g_cfg_slotdrive >= 0) {
			cfg_file_draw();
		}

		key = -1;
		while(g_config_control_panel) {
			video_update();
			key = adb_read_c000();
			if(key & 0x80) {
				key = key & 0x7f;
				(void)adb_access_c010();
				break;
			} else {
				key = -1;
			}
			micro_sleep(1.0/60.0);
			g_cfg_vbl_count++;
			if(!match_found) {
				break;
			}
		}

		if((key >= 0) && (g_cfg_slotdrive < 0)) {
			// Normal menu system
			switch(key) {
			case 0x0a: /* down arrow */
				menu_line++;
				menu_inc = 1;
				break;
			case 0x0b: /* up arrow */
				menu_line--;
				menu_inc = 0;
				if(menu_line < 1) {
					menu_line = 1;
				}
				break;
			case 0x15: /* right arrow */
				cfg_parse_menu(menu_ptr, menu_line,menu_line,1);
				break;
			case 0x08: /* left arrow */
				cfg_parse_menu(menu_ptr,menu_line,menu_line,-1);
				break;
			case 0x0d:
				type = menu_ptr[menu_line].cfgtype;
				ptr = menu_ptr[menu_line].ptr;
				switch(type & 0xf) {
				case CFGTYPE_MENU:
					menu_ptr = (Cfg_menu *)ptr;
					menu_line = 1;
					break;
				case CFGTYPE_DISK:
					g_cfg_slotdrive = type >> 4;
					cfg_file_init();
					break;
				case CFGTYPE_FUNC:
					fn_ptr = (void (*)())ptr;
					(*fn_ptr)();
					break;
				}
				break;
			case 0x1b:
				// Jump to last menu entry
				menu_line = max_line;
				break;
			case 'e':
			case 'E':
				type = menu_ptr[menu_line].cfgtype;
				if((type & 0xf) == CFGTYPE_DISK) {
					eject_disk_by_num(type >> 12,
							(type >> 4) & 0xff);
				}
				break;
			default:
				printf("key: %02x\n", key);
			}
		} else if(key >= 0) {
			cfg_file_handle_key(key);
		}
	}

	for(i = 0; i < 0x400; i++) {
		set_memory_c(0xe00400+i, g_save_text_screen_bytes[i], 0);
		set_memory_c(0xe10400+i, g_save_text_screen_bytes[0x400+i], 0);
	}

	// And quit
	g_config_control_panel = 0;
	g_adb_repeat_vbl = g_vbl_count + 60;
	g_cur_a2_stat = g_save_cur_a2_stat;
	change_display_mode(g_cur_dcycs);
	g_full_refresh_needed = -1;
	g_a2_screen_buffer_changed = -1;
}


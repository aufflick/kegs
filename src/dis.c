/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_dis_c[] = "@(#)$KmKId: dis.c,v 1.90 2003-11-18 17:35:30-05 kentd Exp $";

#include <stdio.h>
#include "defc.h"
#include <stdarg.h>

#include "disas.h"

#define LINE_SIZE 160

extern byte *g_memory_ptr;
extern byte *g_slow_memory_ptr;
extern byte *g_rom_fc_ff_ptr;
extern byte *g_rom_cards_ptr;
extern word32 g_mem_size_base, g_mem_size_exp;
extern int halt_sim;
extern int enter_debug;
extern int statereg;
extern word32 stop_run_at;
extern int stop_on_c03x;
extern int Verbose;
extern int Halt_on;
extern int g_rom_version;

extern int g_testing_enabled;

int	g_num_breakpoints = 0;
word32	g_breakpts[MAX_BREAK_POINTS];

extern int g_irq_pending;

extern Engine_reg engine;

#define W_BUF_LEN	128
char w_buff[W_BUF_LEN];

int g_stepping = 0;

word32	list_kpc;
int	hex_line_len;
word32	a1,a2,a3,a4;
int a1bank, a2bank, a3bank, a4bank;
char *line_ptr;
int mode,old_mode;
int got_num;

int	g_quit_sim_now = 0;

int
get_num()
{
	int tmp1;

	a2 = 0;
	got_num = 0;
	while(1) {
		if(mode == 0 && got_num != 0) {
/*
			printf("In getnum, mode =0, setting a1,a3 = a2\n");
			printf("a2: %x\n", a2);
*/
			a3 = a2;
			a3bank = a2bank;
			a1 = a2;
			a1bank = a2bank;
		}
		tmp1 = *line_ptr++ & 0x7f;
		if(tmp1 >= '0' && tmp1 <= '9') {
			a2 = (a2 << 4) + tmp1 - '0';
			got_num = 1;
			continue;
		}
		if(tmp1 >= 'a' && tmp1 <= 'f') {
			a2 = (a2 << 4) + tmp1 - 'a' + 10;
			got_num = 1;
			continue;
		}
		if(tmp1 == '/') {
			a2bank = a2;
			a2 = 0;
			continue;
		}
		return tmp1;
	}
}

void
debugger_help()
{
	printf("KEGS Debugger help (courtesy Fredric Devernay\n");
	printf("General command syntax: [bank]/[address][command]\n");
	printf("e.g. 'e1/0010B' to set a breakpoint at the interrupt jump pt\n");
	printf("Enter all addresses using lower-case\n");
	printf("As with the IIgs monitor, you can omit the bank number after\n");
	printf("having set it: 'e1/0010B' followed by '14B' will set\n");
	printf("breakpoints at e1/0010 and e1/0014\n");
	printf("\n");
	printf("g                       Go\n");
	printf("[bank]/[addr]g          Go from [bank]/[address]\n");
	printf("s                       Step one instruction\n");
	printf("[bank]/[addr]s          Step one instr at [bank]/[address]\n");
	printf("[bank]/[addr]B          Set breakpoint at [bank]/[address]\n");
	printf("B                       Show all breakpoints\n");
	printf("[bank]/[addr]D          Delete breakpoint at [bank]/[address]\n");
	printf("[bank]/[addr1].[addr2]  View memory\n");
	printf("[bank]/[addr]L          Disassemble memory\n");

	printf("P                       Dump the trace to 'pc_log_out'\n");
	printf("Z                       Dump SCC state\n");
	printf("I                       Dump IWM state\n");
	printf("[drive].[track]I        Dump IWM state\n");
	printf("E                       Dump Ensoniq state\n");
	printf("[osc]E                  Dump oscillator [osc] state\n");
	printf("R                       Dump dtime array and events\n");
	printf("T                       Show toolbox log\n");
	printf("[bank]/[addr]T          Dump tools using ptr [bank]/[addr]\n");
	printf("                            as 'tool_set_info'\n");
	printf("[mode]V                 XOR verbose with 1=DISK, 2=IRQ,\n");
	printf("                         4=CLK,8=SHADOW,10=IWM,20=DOC,\n");
	printf("                         40=ABD,80=SCC, 100=TEST, 200=VIDEO\n");
	printf("[mode]H                 XOR halt_on with 1=SCAN_INT,\n");
	printf("                         2=IRQ, 4=SHADOW_REG, 8=C70D_WRITES\n");
	printf("r                       Reset\n");
	printf("[0/1]=m                 Changes m bit for l listings\n");
	printf("[0/1]=x                 Changes x bit for l listings\n");
	printf("[t]=z                   Stops at absolute time t (obsolete)\n");
	printf("S                       show_bankptr_bank0 & smartport errs\n");
	printf("P                       show_pmhz\n");
	printf("A                       show_a2_line_stuff show_adb_log\n");
	printf("Ctrl-e                  Dump registers\n");
	printf("[bank]/[addr1].[addr2]us[file]  Save mem area to [file]\n");
	printf("[bank]/[addr1].[addr2]ul[file]  Load mem area from [file]\n");
	printf("v			Show video information\n");
	printf("q                       Exit Debugger (and KEGS)\n");
}

void
do_debug_intfc()
{
	char	linebuf[LINE_SIZE];
	int	slot_drive;
	int	track;
	int	osc;
	int	done;
	int	ret_val;

	hex_line_len = 0x10;
	a1 = 0; a2 = 0; a3 = 0; a4 = 0;
	a1bank = 0; a2bank = 0; a3bank = 0; a4bank = 0;
	list_kpc = engine.kpc;
	g_stepping = 0;
	mode = 0; old_mode = 0;
	done = 0;
	stop_run_at = -1;

	x_auto_repeat_on(0);

	if(g_quit_sim_now) {
		printf("Exiting immediately\n");
		return;
	}

	printf("Type 'h' for help\n");

	while(!done) {
		printf("> "); fflush(stdout);
		if(read_line(linebuf,LINE_SIZE-1) <= 0) {
			done = 1;
			continue;
		}
		line_ptr = linebuf;

/*
		printf("input line: :%s:\n", linebuf);
		printf("mode: %d\n", mode);
*/
		mode = 0;

		while(*line_ptr != 0) {
			ret_val = get_num();
/*
			printf("ret_val: %x, got_num= %d\n", ret_val,
				got_num);
*/
			old_mode = mode;
			mode = 0;
			switch(ret_val) {
			case 'h':
				debugger_help();
				break;
			case 'R':
				show_dtime_array();
				show_all_events();
				break;
			case 'I':
				slot_drive = -1;
				track = -1;
				if(got_num) {
					if(old_mode == '.') {
						slot_drive = a1;
					}
					track = a2;
				}
				iwm_show_track(slot_drive, track);
				iwm_show_stats();
				break;
			case 'E':
				osc = -1;
				if(got_num) {
					osc = a2;
				}
				doc_show_ensoniq_state(osc);
				break;
			case 'T':
				if(got_num) {
					show_toolset_tables(a2bank, a2);
				} else {
					show_toolbox_log();
				}
				break;
			case 'v':
				video_show_debug_info();
				break;
			case 'V':
				printf("g_irq_pending: %d\n", g_irq_pending);
				printf("Setting Verbose ^= %04x\n", a1);
				Verbose ^= a1;
				printf("Verbose is now: %04x\n", Verbose);
				break;
			case 'H':
				printf("Setting Halt_on ^= %04x\n", a1);
				Halt_on ^= a1;
				printf("Halt_on is now: %04x\n", Halt_on);
				break;
			case 'r':
				do_reset();
				list_kpc = engine.kpc;
				break;
			case 'm':
				if(old_mode == '=') {
					if(!a1) {
						engine.psr &= ~0x20;
					} else {
						engine.psr |= 0x20;
					}
					if(engine.psr & 0x100) {
						engine.psr |= 0x30;
					}
				}
				break;
			case 'x':
				if(old_mode == '=') {
					if(!a1) {
						engine.psr &= ~0x10;
					} else {
						engine.psr |= 0x10;
					}
					if(engine.psr & 0x100) {
						engine.psr |= 0x30;
					}
				}
				break;
			case 'z':
				if(old_mode == '=') {
					stop_run_at = a1;
					printf("Calling add_event for t:%08x\n",
						a1);
					add_event_stop((double)a1);
					printf("set stop_run_at = %x\n", a1);
				}
				break;
			case 'l': case 'L':
				do_debug_list();
				break;
			case 'Z':
				show_scc_log();
				show_scc_state();
				break;
			case 'S':
				show_bankptrs_bank0rdwr();
				smartport_error();
				break;
			case 'C':
				show_xcolor_array();
				break;
			case 'P':
				show_pc_log();
				break;
			case 'M':
				show_pmhz();
				break;
			case 'A':
				show_a2_line_stuff();
				show_adb_log();
				break;
			case 's':
				g_stepping = 1;
				if(got_num) {
					engine.kpc = (a2bank<<16) + (a2&0xffff);
				}
				mode = 's';
				list_kpc = engine.kpc;
				break;
			case 'B':
				if(got_num) {
					printf("got_num: %d, a2bank: %x, a2: %x\n", got_num, a2bank, a2);
					set_bp((a2bank << 16) + a2);
				} else {
					show_bp();
				}
				break;
			case 'D':
				if(got_num) {
					printf("got_num: %d, a2bank: %x, a2: %x\n", got_num, a2bank, a2);
					delete_bp((a2bank << 16) + a2);
				}
				break;
			case 'g':
			case 'G':
				printf("Going..\n");
				g_stepping = 0;
				if(got_num) {
					engine.kpc = (a2bank<<16) + (a2&0xffff);
				}
				if(ret_val == 'G' && g_testing_enabled) {
					do_gen_test(got_num, a2);
				} else {
					do_go();
				}
				list_kpc = engine.kpc;
				break;
			case 'q':
			case 'Q':
				printf("Exiting debugger\n");
				return;
				break;
			case 'u':
				printf("Unix commands\n");
				do_debug_unix();
				break;
			case ':': case '.':
			case '+': case '-':
			case '=': case ',':
				mode = ret_val;
				printf("Setting mode = %x\n", mode);
				break;
			case ' ': case '\t':
				if(!got_num) {
					mode = old_mode;
					break;
				}
				do_blank();
				break;
			case 0x05: /* ctrl-e */
				show_regs();
				break;
			case '\n':
				*line_ptr = 0;
				if(old_mode == 's') {
					do_blank();
					break;
				}
				if(line_ptr == &linebuf[1]) {
					a2 = a1 | (hex_line_len - 1);
					show_hex_mem(a1bank,a1,a2bank,a2, -1);
					a1 = a2 + 1;
				} else {
					if(got_num == 1 || mode == 's') {
						do_blank();
					}
				}
				break;
			case 'w':
				read_line(w_buff, W_BUF_LEN);
				break;
			case 'X':
				stop_on_c03x = !stop_on_c03x;
				printf("stop_on_c03x set to %d\n",stop_on_c03x);
				break;
			default:
				printf("\nUnrecognized command: %s\n",linebuf);
				*line_ptr = 0;
				break;
			}
		}

	}
	printf("Console closed.\n");
}

word32
dis_get_memory_ptr(word32 addr)
{
	word32	tmp1, tmp2, tmp3;

	tmp1 = get_memory_c(addr, 0);
	tmp2 = get_memory_c(addr + 1, 0);
	tmp3 = get_memory_c(addr + 2, 0);

	return (tmp3 << 16) + (tmp2 << 8) + tmp1;
}

void
show_one_toolset(FILE *toolfile, int toolnum, word32 addr)
{
	word32	rout_addr;
	int	num_routs;
	int	i;

	num_routs = dis_get_memory_ptr(addr);
	fprintf(toolfile, "Tool 0x%02x, table: 0x%06x, num_routs:%03x\n",
		toolnum, addr, num_routs);

	for(i = 1; i < num_routs; i++) {
		rout_addr = dis_get_memory_ptr(addr + 4*i);
		fprintf(toolfile, "%06x = %02x%02x\n", rout_addr, i, toolnum);
	}
}

void
show_toolset_tables(word32 a2bank, word32 addr)
{
	FILE	*toolfile;
	word32	tool_addr;
	int	num_tools;
	int	i;

	addr = (a2bank << 16) + (addr & 0xffff);

	toolfile = fopen("tool_set_info", "wt");
	if(toolfile == 0) {
		fprintf(stderr, "fopen of tool_set_info failed: %d\n", errno);
		exit(2);
	}

	num_tools = dis_get_memory_ptr(addr);
	fprintf(toolfile, "There are 0x%02x tools using ptr at %06x\n",
			num_tools, addr);

	for(i = 1; i < num_tools; i++) {
		tool_addr = dis_get_memory_ptr(addr + 4*i);
		show_one_toolset(toolfile, i, tool_addr);
	}

	fclose(toolfile);
}


#ifndef TEST65
void
do_gen_test(int got_num, int base_seed)
{
	/* dummy */
}
#endif

void
set_bp(word32 addr)
{
	int	count;

	printf("About to set BP at %06x\n", addr);
	count = g_num_breakpoints;
	if(count >= MAX_BREAK_POINTS) {
		printf("Too many (0x%02x) breakpoints set!\n", count);
		return;
	}

	g_breakpts[count] = addr;
	g_num_breakpoints = count + 1;
	fixup_brks();
}

void
show_bp()
{
	int i;

	printf("Showing breakpoints set\n");
	for(i = 0; i < g_num_breakpoints; i++) {
		printf("bp:%02x: %06x\n", i, g_breakpts[i]);
	}
}

void
delete_bp(word32 addr)
{
	int	count;
	int	hit;
	int	i;

	printf("About to delete BP at %06x\n", addr);
	count = g_num_breakpoints;

	hit = -1;
	for(i = 0; i < count; i++) {
		if(g_breakpts[i] == addr) {
			hit = i;
			break;
		}
	}

	if(hit < 0) {
		printf("Breakpoint not found!\n");
	} else {
		printf("Deleting brkpoint #0x%02x\n", hit);
		for(i = hit+1; i < count; i++) {
			g_breakpts[i-1] = g_breakpts[i];
		}
		g_num_breakpoints = count - 1;
		setup_pageinfo();
	}

	show_bp();
}

void
do_blank()
{
	int tmp, i;

	switch(old_mode) {
	case 's':
		tmp = a2;
		if(tmp == 0) tmp = 1;
		enter_debug = 0;
		for(i = 0; i < tmp; i++) {
			g_stepping = 1;
			do_step();
			if(enter_debug || halt_sim != 0) {
				if(halt_sim != HALT_EVENT) {
					break;
				}
			}
		}
		list_kpc = engine.kpc;
		/* video_update_through_line(262); */
		break;
	case ':':
		set_memory_c(((a3bank << 16) + a3), a2, 0);
		a3++;
		mode = old_mode;
		break;
	case '.':
	case 0:
		xam_mem(-1);
		break;
	case ',':
		xam_mem(16);
		break;
	case '+':
		printf("%x\n", a1 + a2);
		break;
	case '-':
		printf("%x\n", a1 - a2);
		break;
	default:
		printf("Unknown mode at space: %d\n", old_mode);
		break;
	}
}

void
do_go()
{
	clear_halt();

	run_prog();
	show_regs();
}

void
do_step()
{
	int size;
	int size_mem_imm, size_x_imm;

	clear_halt();

	run_prog();

	show_regs();
	size_mem_imm = 2;
	if(engine.psr & 0x20) {
		size_mem_imm = 1;
	}
	size_x_imm = 2;
	if(engine.psr & 0x10) {
		size_x_imm = 1;
	}
	size = do_dis(stdout, engine.kpc, size_mem_imm, size_x_imm, 0, 0);
}

void
xam_mem(int count)
{
	show_hex_mem(a1bank, a1, a2bank, a2, count);
	a1 = a2 + 1;
}

void
show_hex_mem(int startbank, word32 start, int endbank, word32 end, int count)
{
	char	ascii[MAXNUM_HEX_PER_LINE];
	word32	i;
	int	val, offset;

	if(count < 0) {
		count = 16 - (start & 0xf);
	}

	offset = 0;
	ascii[0] = 0;
	printf("Showing hex mem: bank: %x, start: %x, end: %x\n",
		startbank, start, end);
	for(i = start; i <= end; i++) {
		if( (i==start) || (count == 16) ) {
			printf("%04x:",i);
		}
		printf(" %02x", get_memory_c((startbank <<16) + i, 0));
		val = get_memory_c((startbank << 16) + i, 0) & 0x7f;
		if(val < 32 || val >= 0x7f) {
			val = '.';
		}
		ascii[offset++] = val;
		ascii[offset] = 0;
		count--;
		if(count <= 0) {
			printf("   %s\n", ascii);
			offset = 0;
			ascii[0] = 0;
			count = 16;
		}
	}
	if(offset > 0) {
		printf("   %s\n", ascii);
	}
}


int
read_line(char *buf, int len)
{
	int space_left;
	int ret;

	space_left = len;

	ret = 0;
	buf[0] = 0;
	while(space_left > 0) {
		ret = read(0,buf,1);
		if(ret <= 0) {
			printf("read <= 0\n");
			return(len-space_left);
		}
		space_left -= ret;
		if(buf[ret-1] == 0x0a) {
			return(len-space_left);
		}
		buf = &buf[ret];
	}
	return(len-space_left);
}

void
do_debug_list()
{
	int i;
	int size;
	int size_mem_imm, size_x_imm;

	if(got_num) {
		list_kpc = (a2bank << 16) + (a2 & 0xffff);
	}
	printf("%d=m %d=x %d=LCBANK\n", (engine.psr >> 5)&1,
		(engine.psr >> 4) & 1, (statereg & 0x4) >> 2);
	
	size_mem_imm = 2;
	if(engine.psr & 0x20) {
		size_mem_imm = 1;
	}
	size_x_imm = 2;
	if(engine.psr & 0x10) {
		size_x_imm = 1;
	}
	for(i=0;i<20;i++) {
		size = do_dis(stdout, list_kpc, size_mem_imm,
			size_x_imm, 0, 0);
		list_kpc += size;
	}
}

const char *g_kegs_rom_names[] = { "ROM", "ROM.01", "ROM.03", 0 };

const char *g_kegs_c1rom_names[] = { 0 };
const char *g_kegs_c2rom_names[] = { 0 };
const char *g_kegs_c3rom_names[] = { 0 };
const char *g_kegs_c4rom_names[] = { 0 };
const char *g_kegs_c5rom_names[] = { 0 };
const char *g_kegs_c6rom_names[] = { "c600.rom", "controller.rom", "disk.rom",
				"DISK.ROM", "diskII.prom", 0 };
const char *g_kegs_c7rom_names[] = { 0 };

const char **g_kegs_rom_card_list[8] = {
	0,			g_kegs_c1rom_names,
	g_kegs_c2rom_names,	g_kegs_c3rom_names,
	g_kegs_c4rom_names,	g_kegs_c5rom_names,
	g_kegs_c6rom_names,	g_kegs_c7rom_names };

byte g_rom_c600_rom01_diffs[256] = {
	0x00, 0x00, 0x00, 0x00, 0xc6, 0x00, 0xe2, 0x00,
	0xd0, 0x50, 0x0f, 0x77, 0x5b, 0xb9, 0xc3, 0xb1,
	0xb1, 0xf8, 0xcb, 0x4e, 0xb8, 0x60, 0xc7, 0x2e,
	0xfc, 0xe0, 0xbf, 0x1f, 0x66, 0x37, 0x4a, 0x70,
	0x55, 0x2c, 0x3c, 0xfc, 0xc2, 0xa5, 0x08, 0x29,
	0xac, 0x21, 0xcc, 0x09, 0x55, 0x03, 0x17, 0x35,
	0x4e, 0xe2, 0x0c, 0xe9, 0x3f, 0x9d, 0xc2, 0x06,
	0x18, 0x88, 0x0d, 0x58, 0x57, 0x6d, 0x83, 0x8c,
	0x22, 0xd3, 0x4f, 0x0a, 0xe5, 0xb7, 0x9f, 0x7d,
	0x2c, 0x3e, 0xae, 0x7f, 0x24, 0x78, 0xfd, 0xd0,
	0xb5, 0xd6, 0xe5, 0x26, 0x85, 0x3d, 0x8d, 0xc9,
	0x79, 0x0c, 0x75, 0xec, 0x98, 0xcc, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x39, 0x00, 0x35, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
	0x6c, 0x44, 0xce, 0x4c, 0x01, 0x08, 0x00, 0x00
};

void
load_roms()
{
	char	name_buf[256];
	struct stat stat_buf;
	const char **names_ptr;
	int	more_than_8mb;
	int	changed_rom;
	int	len;
	int	fd;
	int	ret;
	int	i;

	g_rom_version = 0;
	setup_kegs_file(&name_buf[0], (int)sizeof(name_buf), 0,
							&g_kegs_rom_names[0]);
	fd = open(name_buf, O_RDONLY | O_BINARY);
	if(fd < 0) {
		printf("Open ROM file %s failed:%d, errno:%d\n", name_buf, fd,
				errno);
		my_exit(-3);
	}

	ret = fstat(fd, &stat_buf);
	if(ret != 0) {
		fprintf(stderr, "fstat returned %d on fd %d, errno: %d\n",
			ret, fd, errno);
		my_exit(2);
	}

	len = stat_buf.st_size;
	if(len == 128*1024) {
		g_rom_version = 1;
		g_mem_size_base = 256*1024;
		ret = read(fd, &g_rom_fc_ff_ptr[2*65536], len);
	} else if(len == 256*1024) {
		g_rom_version = 3;
		g_mem_size_base = 1024*1024;
		ret = read(fd, &g_rom_fc_ff_ptr[0], len);
	} else {
		fprintf(stderr, "ROM size %d not 128K or 256K\n", len);
		my_exit(4);
	}

	printf("Read: %d bytes of ROM\n", ret);
	if(ret != len) {
		printf("errno: %d\n", errno);
		my_exit(-3);
	}
	close(fd);

	memset(&g_rom_cards_ptr[0], 0, 256*16);

	/* initialize c600 rom to be diffs from the real ROM, to build-in */
	/*  Apple II compatibility without distributing ROMs */
	for(i = 0; i < 256; i++) {
		g_rom_cards_ptr[0x600 + i] = g_rom_fc_ff_ptr[0x3c600 + i] ^
				g_rom_c600_rom01_diffs[i];
	}
	if(g_rom_version >= 3) {
		/* some patches */
		g_rom_cards_ptr[0x61b] ^= 0x40;
		g_rom_cards_ptr[0x61c] ^= 0x33;
		g_rom_cards_ptr[0x632] ^= 0xc0;
		g_rom_cards_ptr[0x633] ^= 0x33;
	}

	for(i = 1; i < 8; i++) {
		names_ptr = g_kegs_rom_card_list[i];
		if(names_ptr == 0) {
			continue;
		}
		if(*names_ptr == 0) {
			continue;
		}

		setup_kegs_file(&name_buf[0], (int)sizeof(name_buf), 1,
								names_ptr);

		if(name_buf[0] != 0) {
			fd = open(name_buf, O_RDONLY | O_BINARY);
			if(fd < 0) {
				printf("Open card ROM file %s failed: %d "
					"err:%d\n", name_buf, fd, errno);
				my_exit(-3);
			}

			len = 256;
			ret = read(fd, &g_rom_cards_ptr[i*0x100], len);

			if(ret != len) {
				printf("errno: %d, expected %d, got %d\n",
					errno, len, ret);
				my_exit(-3);
			}
			close(fd);
		}
	}

	more_than_8mb = (g_mem_size_exp > 0x800000);
	/* Only do the patch if users wants more than 8MB of expansion mem */

	changed_rom = 0;
	if(g_rom_version == 1) {
		/* make some patches to ROM 01 */
#if 0
		/* 1: Patch ROM selftest to not do speed test */
		printf("Patching out speed test failures from ROM 01\n");
		g_rom_fc_ff_ptr[0x3785a] = 0x18;
		changed_rom = 1;
#endif

#if 0
		/* 2: Patch ROM selftests not to do tests 2,4 */
		/* 0 = skip, 1 = do it, test 1 is bit 0 of LSByte */
		g_rom_fc_ff_ptr[0x371e9] = 0xf5;
		g_rom_fc_ff_ptr[0x371ea] = 0xff;
		changed_rom = 1;
#endif

		if(more_than_8mb) {
			/* Geoff Weiss patch to use up to 14MB of RAM */
			g_rom_fc_ff_ptr[0x30302] = 0xdf;
			g_rom_fc_ff_ptr[0x30314] = 0xdf;
			g_rom_fc_ff_ptr[0x3031c] = 0x00;
			changed_rom = 1;
		}

		/* Patch ROM selftest to not do ROM cksum if any changes*/
		if(changed_rom) {
			g_rom_fc_ff_ptr[0x37a06] = 0x18;
			g_rom_fc_ff_ptr[0x37a07] = 0x18;
		}
	} else if(g_rom_version == 3) {
		/* patch ROM 03 */
		printf("Patching ROM 03 smartport bug\n");
		/* 1: Patch Smartport code to fix a stupid bug */
		/*   that causes it to write the IWM status reg into c036, */
		/*   which is the system speed reg...it's "safe" since */
		/*   IWM status reg bit 4 must be 0 (7MHz)..., otherwise */
		/*   it might have turned on shadowing in all banks! */
		g_rom_fc_ff_ptr[0x357c9] = 0x00;
		changed_rom = 1;

#if 0
		/* patch ROM 03 to not to speed test */
		/*  skip fast speed test */
		g_rom_fc_ff_ptr[0x36ad7] = 0x18;
		g_rom_fc_ff_ptr[0x36ad8] = 0x18;
		changed_rom = 1;
#endif

#if 0
		/*  skip slow speed test */
		g_rom_fc_ff_ptr[0x36ae7] = 0x18;
		g_rom_fc_ff_ptr[0x36ae8] = 0x6b;
		changed_rom = 1;
#endif

#if 0
		/* 4: Patch ROM 03 selftests not to do tests 1-4 */
		g_rom_fc_ff_ptr[0x364a9] = 0xf0;
		g_rom_fc_ff_ptr[0x364aa] = 0xff;
		changed_rom = 1;
#endif

		/* ROM tests are in ff/6403-642x, where 6403 = addr of */
		/*  test 1, etc. */

		if(more_than_8mb) {
			/* Geoff Weiss patch to use up to 14MB of RAM */
			g_rom_fc_ff_ptr[0x30b] = 0xdf;
			g_rom_fc_ff_ptr[0x31d] = 0xdf;
			g_rom_fc_ff_ptr[0x325] = 0x00;
			changed_rom = 1;
		}

		if(changed_rom) {
			/* patch ROM 03 selftest to not do ROM cksum */
			g_rom_fc_ff_ptr[0x36cb0] = 0x18;
			g_rom_fc_ff_ptr[0x36cb1] = 0x18;
		}

	}
}

void
do_debug_unix()
{
	char	localbuf[LINE_SIZE];
	word32	offset, len;
	int	fd, ret;
	int	load, save;
	int	i;

	load = 0; save = 0;
	switch(*line_ptr++) {
	case 'l': case 'L':
		printf("Loading..");
		load = 1;
		break;
	case 's': case 'S':
		printf("Saving...");
		save = 1;
		break;
	default:
		printf("Unknown unix command: %c\n", *(line_ptr-1));
		*line_ptr = 0;
		return;
	}
	while(*line_ptr == ' ' || *line_ptr == '\t') {
		line_ptr++;
	}
	i = 0;
	while(i < LINE_SIZE) {
		localbuf[i++] = *line_ptr++;
		if(*line_ptr==' ' || *line_ptr=='\t' || *line_ptr == '\n') {
			break;
		}
	}
	localbuf[i] = 0;
	

	printf("About to open: %s,len: %d\n", localbuf, (int)strlen(localbuf));
	if(load) {
		fd = open(localbuf,O_RDONLY | O_BINARY);
	} else {
		fd = open(localbuf,O_WRONLY | O_CREAT | O_BINARY, 0x1b6);
	}
	if(fd < 0) {
		printf("Open %s failed: %d\n", localbuf, fd);
		printf("errno: %d\n", errno);
		return;
	}
	if(load) {
		offset = a1 & 0xffff;
		len = 0x20000 - offset;
	} else {
		if(old_mode == '.') {
			len = a2 - a1 + 1;
		} else {
			len = 0x100;
		}
	}
	if(load) {
		if(a1bank >= 0xe0 && a1bank < 0xe2) {
			ret = read(fd,&g_slow_memory_ptr[((a1bank & 1)<<16)+a1],len);
		} else {
			ret = read(fd,&g_memory_ptr[(a1bank << 16) + a1],len);
		}
	} else {
		if(a1bank >= 0xe0 && a1bank < 0xe2) {
			ret = write(fd,&g_slow_memory_ptr[((a1bank & 1)<<16)+a1],len);
		} else {
			ret = write(fd,&g_memory_ptr[(a1bank << 16) + a1],len);
		}
	}
	printf("Read/write: addr %06x for %04x bytes, ret: %x bytes\n",
		(a1bank << 16) + a1, len, ret);
	if(ret < 0) {
		printf("errno: %d\n", errno);
	}
	a1 = a1 + ret;
}

void
do_debug_load()
{
	printf("Sorry, can't load now\n");
}


int
do_dis(FILE *outfile, word32 kpc, int accsize, int xsize,
	int op_provided, word32 instr)
{
	char	buffer[150];
	const char *out;
	int	args, type;
	int	opcode;
	word32	val;
	word32	oldkpc;
	word32	dtype;
	int	signed_val;

	oldkpc = kpc;
	if(op_provided) {
		opcode = (instr >> 24) & 0xff;
	} else {
		opcode = (int)get_memory_c(kpc, 0) & 0xff;
	}

	kpc++;

	dtype = disas_types[opcode];
	out = disas_opcodes[opcode];
	type = dtype & 0xff;
	args = dtype >> 8;

	if(args > 3) {
		if(args == 4) {
			args = accsize;
		} else if(args == 5) {
			args = xsize;
		}
	}

	val = -1;
	switch(args) {
	case 0:
		val = 0;
		break;
	case 1:
		if(op_provided) {
			val = instr & 0xff;
		} else {
			val = get_memory_c(kpc, 0);
		}
		break;
	case 2:
		if(op_provided) {
			val = instr & 0xffff;
		} else {
			val = get_memory16_c(kpc, 0);
		}
		break;
	case 3:
		if(op_provided) {
			val = instr & 0xffffff;
		} else {
			val = get_memory24_c(kpc, 0);
		}
		break;
	default:
		fprintf(stderr, "args out of rang: %d, opcode: %08x\n",
			args, opcode);
		break;
	}
	kpc += args;

	if(!op_provided) {
		instr = (opcode << 24) | (val & 0xffffff);
	}

	switch(type) {
	case ABS:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%04x",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case ABSX:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%04x,X",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case ABSY:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%04x,Y",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case ABSLONG:
		if(args != 3) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%06x",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case ABSIND:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t($%04x)",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case ABSXIND:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t($%04x,X)",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case IMPLY:
		if(args != 0) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s",out);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case ACCUM:
		if(args != 0) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s",out);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case IMMED:
		if(args == 1) {
			sprintf(buffer,"%s\t#$%02x",out,val);
		} else if(args == 2) {
			sprintf(buffer,"%s\t#$%04x",out,val);
		} else {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case JUST8:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%02x",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOC:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%02x",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCX:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%02x,X",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCY:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%02x,Y",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case LONG:
		if(args != 3) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%06x",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case LONGX:
		if(args != 3) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%06x,X",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCIND:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t($%02x)",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCINDY:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t($%02x),Y",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCXIND:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t($%02x,X)",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCBRAK:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t[$%02x]",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DLOCBRAKY:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t[$%02x],y",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DISP8:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		signed_val = (signed char)val;
		sprintf(buffer,"%s\t$%04x",out,
				(word32)(kpc+(signed_val)) & 0xffff);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DISP8S:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%02x,S",out,(word32)(byte)(val));
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DISP8SINDY:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t($%02x,S),Y",out,(word32)(byte)(val));
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case DISP16:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%04x", out,
				(word32)(kpc+(signed)(word16)(val)) & 0xffff);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case MVPMVN:
		if(args != 2) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t$%02x,$%02x",out,val&0xff,val>>8);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	case SEPVAL:
	case REPVAL:
		if(args != 1) {
			printf("arg # mismatch for opcode %x\n", opcode);
		}
		sprintf(buffer,"%s\t#$%02x",out,val);
		show_line(outfile, oldkpc,instr,args+1,buffer);
		break;
	default:
		printf("argument type: %d unexpected\n", type);
		break;
	}

	return(args+1);
}

void
show_line(FILE *outfile, word32 kaddr, word32 operand, int size,
	char *string)
{
	int i;
	int opcode;

	fprintf(outfile, "%02x/%04x: ", kaddr >> 16, kaddr & 0xffff);
	opcode = (operand >> 24) & 0xff;
	fprintf(outfile,"%02x ", opcode);

	for(i=1;i<size;i++) {
		fprintf(outfile, "%02x ", operand & 0xff);
		operand = operand >> 8;
	}
	for(;i<5;i++) {
		fprintf(outfile, "   ");
	}
	fprintf(outfile,"%s\n", string);
}

void
halt_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	set_halt(1);
}

void
halt2_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	set_halt(2);
}


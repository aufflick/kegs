/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

#ifdef INCLUDE_RCSID_C
const char rcsid_scc_h[] = "@(#)$KmKId: scc.h,v 1.12 2003-11-21 00:27:00-05 kentd Exp $";
#endif

#ifdef _WIN32
# include <winsock.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#if defined(HPUX) || defined(__linux__) || defined(SOLARIS) || defined(MAC) || defined(__MACH__)
# define SCC_SOCKETS
#endif


/* my scc port 0 == channel A, port 1 = channel B */

#define	SCC_INBUF_SIZE		4096		/* must be a power of 2 */
#define	SCC_OUTBUF_SIZE		4096		/* must be a power of 2 */

STRUCT(Scc) {
	int	port;
	int	state;
	int	accfd;
	int	rdwrfd;
	void	*host_handle;
	void	*host_handle2;
	int	host_aux1;
	int	read_called_this_vbl;
	int	write_called_this_vbl;

	int	mode;
	int	reg_ptr;
	int	reg[16];

	int	rx_queue_depth;
	byte	rx_queue[4];

	int	in_rdptr;
	int	in_wrptr;
	byte	in_buf[SCC_INBUF_SIZE];

	int	out_rdptr;
	int	out_wrptr;
	byte	out_buf[SCC_OUTBUF_SIZE];

	int	br_is_zero;
	int	tx_buf_empty;
	int	wantint_rx;
	int	wantint_tx;
	int	wantint_zerocnt;
	int	int_pending_rx;
	int	int_pending_tx;
	int	int_pending_zerocnt;

	double	br_dcycs;
	double	tx_dcycs;
	double	rx_dcycs;

	int	br_event_pending;
	int	rx_event_pending;
	int	tx_event_pending;

	int	char_size;
	int	baud_rate;
};


// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	jtag.c -- JTAG control

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include <fcntl.h>
#include "pci.h"
#include "jtag.h"
//
//-------------------------------
//			JTAG definitions
//-------------------------------
//
// instruction opcodes und register length stehen in
//		\Xilinx\xc18v00\data\xc18v0?.bsd
//
#define C_JTG_DEVS	6

#define TDI	0x01
#define TMS	0x02

#define BYPASS		0xFF
#define IDCODE		0xFE
#define ISPEN		0xE8
#define FPGM		0xEA	// Programs specific bit values at specified addresses
#define FADDR		0xEB	// Sets the PROM array address register
#define FVFY1		0xF8	// Reads the fuse values at specified addresses
#define NORMRST	0xF0	// Exits ISP Mode ?
#define FERASE		0xEC	// Erases a specified program memory block
#define SERASE		0x0A	// Globally refines the programmed values in the array
#define FDATA0		0xED	// Accesses the array word-line register ?
#define FDATA3		0xF3	// 6
#define CONFIG		0xEE

typedef struct {
	u_int		num_devs;
	u_int		select;
	int		state;		// -1: unknown device (corrupted JTAG chain)
								//  0: state is RTI (RUN-TEST/IDLE)
								//  1: state is CAPTURE-DR
	struct {
		u_long	idcode;
		u_int		ir_len;	// instruction register
	} devs[C_JTG_DEVS];
} JTAG_TAB;

#pragma option -a1
typedef struct {
	u_char	bc;
	u_short	addr;
	u_char	type;
	u_char	data[0x20+1];
} MCS_RECORD;
#pragma option -a

static u_int	(*jt_getcsr)(void);	// get control/status
static void		(*jt_putcsr)(u_int);	// put control/status
#define JT_C	0x300			// autoclock, enable

static u_long	(*jt_data)(void);	// data in
static JT_CONF	*jt_conf;
static u_char	jt_mode;

static JTAG_TAB	jtag_tab;
static int			mcs_hdl=-1;
static int			mcs_ix;
static int			mcs_blen;
static u_char		mcs_buf[0x1000];
static FILE			*svfdat=0;
static char			svf_line[300];
static u_int		svf_ix;
static u_long		svf_value;

static u_int		sc_line;				// SVF file, line number
static u_int		sc_command;
static u_int		sc_length;			// number of bits for svf command
static u_char		dr_tdi[0x1000];	// DR values
static u_char		dr_itdo[0x1000];
static u_char		dr_tdo[0x1000];
static u_char		dr_mask[0x1000];
static u_long		lvdr_tdi;			// DR long value
static u_long		lvdr_tdo;
static u_long		lvdr_mask;
static u_int		lvir_tdi;			// IR long value
static u_int		lvir_tdo;
static u_int		lvir_mask;
static u_char		tdo_val;
#define SC_UNDEF		0
#define SC_HIR			1
#define SC_TIR			2
#define SC_HDR			3
#define SC_TDR			4
#define SC_SIR			5
#define SC_SDR			6
#define SC_RUNTEST	7
#define SC_TDI			8
#define SC_SMASK		9
#define SC_TDO			10
#define SC_MASK		11
#define SC_TCK			12
#define SC_BRACKET_O	13
#define SC_BRACKET_C	14
#define SC_SEMICOLON	15
#define SC_COMMENT	16

static char *const cmd_txt[] ={
	"HIR", "TIR", "HDR", "TDR", "SIR", "SDR", "RUNTEST",
	"TDI", "SMASK", "TDO", "MASK", "TCK", 0
};

static MCS_RECORD	mcs_record;
//
//--------------------------- display_errcode --------------------------------
//
#define CEN_PROMPT		-2		// display no command menu, only prompt
#define CE_PROMPT			2		// display no command menu, only prompt
#define CE_FORMAT			5

static void display_errcode(int err)
{
	if (err == CE_PROMPT) return;

	printf("err# %u, ", err);
	switch (err) {

case CE_FORMAT:
		printf("MCS format error\n"); break;

default:
		printf("\n"); break;
	}
}
//
//===========================================================================
//
//										JTAG Interface
//
//===========================================================================
//
static int svf_a2bin(	// count of valid characters
								// value in svf_value
	char	hex,				// default base
	char	*str)
{
	u_int	t,i;
	char	base;
	u_int	dg;
	u_int	len;

	base=-1; t=0;
	while ((str[t] == ' ') || (str[t] == TAB)) t++;
	for (i=t;; i++) {
		dg=toupper(str[i]);
		if ((dg >= '0') && (dg <= '9')) continue;

		if (   (dg >= 'A') && (dg <= 'F')
			 && (hex || (base == 1))) { base=1; continue; }

		if ((i == 1) && (dg == 'X') && (str[0] == '0')) { base=1; continue; }

		if ((dg == '.') && (base < 0)) { base=0; len=i+1; break; }

		len=i; break;
	}

	if (base < 0) base=hex;

	svf_value=0;
	if (base) {							// Hexa
		for (i=t; i < len; i++) {
			dg=toupper(str[i]);
			if (dg == 'X') continue;

			if (svf_value >= 0x10000000l) return i;
			if (dg >= 'A') dg -=7;

			svf_value =(svf_value <<4) +(dg-'0');
		}

		return len;
	}

	for (i=t; (i < len) && (str[i] != '.'); i++) {
		dg=str[i]-'0';
		if ((svf_value > 429496729l) || ((svf_value == 429496729l) && (dg >= 6)))
			return i;

		svf_value=10*svf_value +dg;
	}

	return len;
}

//--------------------------- svf_readln ------------------------------------
//
static int svf_readln(void)
{
	u_int	i;
	int	c;

	sc_line++; svf_ix=0; i=0;
	while (1) {
		c=fgetc(svfdat);
		if (c == EOF) { svf_line[i]=0; return -1; }

		if ((c == '\r') || (c == '\n')) {
			if (i == 0) continue;

			svf_line[i]=0; return 0;
		}

		if (i >= sizeof(svf_line)-1) {
			printf("line too long\n");
			svf_line[i]=0; return -2;
		}

		svf_line[i++]=c;
	}
}
//
//--------------------------- svf_token -------------------------------------
//
static int svf_token(void)
{
	int	ret;
	int	i;
	char	*const *tkx;
	char	*tk;

	i=0;
	while (1) {
		while ((svf_line[svf_ix] == ' ') || (svf_line[svf_ix] == TAB)) svf_ix++;

		if (!svf_line[svf_ix]) {
			if ((ret=svf_readln()) == 0) continue;

			return ret;
		}

		switch (svf_line[svf_ix]) {
	case '/':
	case '!':
			svf_ix++; return SC_COMMENT;

	case '(':
			svf_ix++; return SC_BRACKET_O;

	case ')':
			svf_ix++; return SC_BRACKET_C;

	case ';':
			svf_ix++; return SC_SEMICOLON;
		}

		ret=SC_HIR; tkx=cmd_txt;
		while ((tk= *tkx++) != 0) {
			i=0;
			while (tk[i] && (tk[i] == svf_line[svf_ix+i])) i++;

			if (!tk[i]) { svf_ix +=i; return ret; }

			ret++; tk++;
		}

		return SC_UNDEF;
	}
}
//
//--------------------------- svf_byte --------------------------------------
//
static int svf_byte(void)
{
	int	ret;
	u_int	i,dg,val;

	val=0;
	for (i=0; i < 2; i++, svf_ix++) {
		if (!svf_line[svf_ix])
			if ((ret=svf_readln()) != 0) return ret;

		dg=toupper(svf_line[svf_ix]);
		if (((dg < '0') || (dg > '9')) && ((dg < 'A') || (dg > 'F'))) return -3;

		if (dg >= 'A') dg -=7;

		val =(val <<4) +(dg-'0');
	}

	return val;
}
//
//--------------------------- MCS_byte --------------------------------------
//
static int MCS_byte(void)
{

	if (mcs_ix >= mcs_blen) {
		if ((mcs_blen == 0) || (mcs_hdl == -1)) return -1;

      mcs_ix=0;
		if ((mcs_blen=read(mcs_hdl, &mcs_buf, sizeof(mcs_buf))) != sizeof(mcs_buf)) {
			if (mcs_blen < 0) { perror("error reading config file"); mcs_blen=0; }

			if (mcs_blen == 0) return -1;
		}
	}

	return mcs_buf[mcs_ix++];
}
//
//--------------------------- MCS_record ------------------------------------
//
// lese naechsten MCS Record nach
//		mcs_record
//
static int MCS_record(void)
{
	int		bx,ix,dx;
	u_char	*bf;

	while ((bx=MCS_byte()) != ':') {
		if (bx < 0) return -1;

		if ((bx != '\r') && (bx != '\n')) {
			printf("illegal Format\n"); return -1;
		}
	}

	ix=0; bf=(u_char*)&mcs_record;
	do {
		bx=MCS_byte();
		if (bx < ' ') break;

		dx =bx -'0';
		if (dx >= 10) dx -=('A'-'9'-1);

		bx=MCS_byte() -'0';
		if (bx >= 10) bx -=('A'-'9'-1);

		bf[ix++]=(dx <<4) |bx;
	} while (ix < sizeof(mcs_record));

	dx=ix-1; bx=0; ix=0;
	do bx +=bf[ix++]; while (ix < dx);
	if (   (mcs_record.bc != (dx-4))
		 || (bf[dx] != (u_char)(-bx))) {
		return CE_FORMAT;
	}

	return 0;
}
//
//--------------------------- jtag_reset ------------------------------------
//
static JTAG_TAB *jtag_reset(void)
{
	u_int		ux,uy;
	u_long	tmp;

	ux=0;
	do {
		uy=0;
		do (*jt_putcsr)(JT_C|TMS); while (++uy < 5);	// TLR state, ID DR wird selectiert
		(*jt_putcsr)(JT_C);									// RTI state
	} while (++ux < 3);						// einmal sollte auch genuegen
//
//--- RTI (Run-Test/Idle)
//
	(*jt_putcsr)(JT_C|TMS);	// Select DR
	(*jt_putcsr)(JT_C);		// Capture DR
	if (jt_mode) (*jt_putcsr)(JT_C);

	ux=C_JTG_DEVS;								// die letzte device ID wird zuerst gelesen
	while (1) {
		uy=0;
		do (*jt_putcsr)(JT_C); while (++uy < 32);	// shift DR
		tmp=(*jt_data)();
		if (   (tmp == 0)						// wurde vorne (TDI) hineingeschoben
			 || (ux == 0)) break;			// maximale Anzahl erreicht

		jtag_tab.devs[--ux].idcode=tmp;
	}

	(*jt_putcsr)(JT_C|TMS);		// Exit1 DR
	(*jt_putcsr)(JT_C|TMS);		// Update DR
	(*jt_putcsr)(JT_C);			// RTI state
	jtag_tab.num_devs=0; jtag_tab.select=0; jtag_tab.state=-1;
	if (   tmp										// Ende nicht erkannt
		 || (ux == C_JTG_DEVS)) return 0;	// kein device vorhanden

	jtag_tab.num_devs=C_JTG_DEVS-ux;
	uy=0;
	do {												// nach vorne schieben
		jtag_tab.devs[uy].idcode=jtag_tab.devs[ux++].idcode;
		jtag_tab.devs[uy++].ir_len=0;			// IR length wird spaeter bestimmt
	} while (ux < C_JTG_DEVS);

	jtag_tab.state=0;
	return &jtag_tab;
}
//
//--------------------------- jtag_select -----------------------------------
//
static int jtag_select(u_int dev)
{
	if (dev >= jtag_tab.num_devs) return CE_PROMPT;

	jtag_tab.select=dev;
	return 0;
}
//
//--------------------------- jtag_runtest ----------------------------------
//
static int jtag_runtest(u_long cnt)
{
	while (cnt) { (*jt_putcsr)(JT_C); cnt--; }
	return 0;
}
//
//--------------------------- jtag_instruction ------------------------------
//
static int jtag_instruction(u_int icode)
{
	u_int		ux,uy;
	u_int		tdo;
	u_char	ms;		// TMS (bit 1)

	if (jtag_tab.state < 0) return -1;

	(*jt_putcsr)(JT_C|TMS|TDI); (*jt_putcsr)(JT_C|TMS|TDI);
	(*jt_putcsr)(JT_C|TDI); (*jt_putcsr)(JT_C|TDI); ux=jtag_tab.num_devs;
	tdo=0; ms=0;
	while (ux) {
		ux--; uy=0;
		do {
			if (   (ux == 0)										// last device
				 && (uy == (jtag_tab.devs[ux].ir_len -1))	// last bit
				) ms=TMS;

			if (ux != jtag_tab.select) {
				(*jt_putcsr)(JT_C|ms|TDI);
//				printf(" %02X", ms|TDI);
			} else {
				tdo |=(((*jt_getcsr)() >>3) &1) <<uy;
//				printf("%02X ", ms|(icode &TDI));
				(*jt_putcsr)(JT_C|ms|(icode &TDI)); icode >>=1;
			}
		} while (++uy < jtag_tab.devs[ux].ir_len);
//		printf(" => %u\n", ux);
	}

	if (icode == CONFIG) { sleep(1); printf("2\n"); }

	(*jt_putcsr)(JT_C|TMS|TDI);
	if (icode == CONFIG) { sleep(1); printf("1\n"); }

	(*jt_putcsr)(JT_C|TDI);
	if (icode == CONFIG) { sleep(1); printf("0\n"); }

	return tdo;
}
//
//--------------------------- jtag_data -------------------------------------
//
static u_long jtag_data(u_long din, u_int len)
{
	u_int		dev,uy;
	u_long	tdo;

	errno=0;
	if (jtag_tab.state < 0) { errno=1; return 0; }

	(*jt_putcsr)(JT_C|TMS|TDI);
	(*jt_putcsr)(JT_C|TDI);
	(*jt_putcsr)(JT_C|TDI);			// SHIFT-DR
	dev=jtag_tab.num_devs; tdo=0;
	while (dev) {
		dev--;
		if (dev != jtag_tab.select) {	// TDI/TDO header and tail
			if (dev == 0) {
				(*jt_putcsr)(JT_C|TMS|TDI);	// letztes device, EXIT1-DR
				break;
			}

			(*jt_putcsr)(JT_C|TDI);
			continue;
		}

		uy=0;
		while (len) {
			len--;
			tdo |=(u_long)(((*jt_getcsr)() >>3) &1) <<uy; uy++;
			if (   (dev == 0)							// letztes device
				 && (len == 0)) {						// letztes bit
				(*jt_putcsr)(JT_C|TMS |((u_char)din &TDI));	// EXIT1-DR
				break;
			}

			(*jt_putcsr)(JT_C|((u_char)din &TDI)); din >>=1;
		}
	}

	(*jt_putcsr)(JT_C|TMS|TDI); (*jt_putcsr)(JT_C|TDI);	// state RTI
	return tdo;
}
//
//--------------------------- jtag_rd_data ----------------------------------
//
static int jtag_rd_data(
	void	*buf,
	u_int	len)		//  0: end
{
	u_long	*bf;
	u_int		ux;

	if (jtag_tab.state < 0) return CE_PROMPT;

	if (len == 0) {
		if (jtag_tab.state == 1) {
			(*jt_putcsr)(JT_C|TMS|TDI);
			(*jt_putcsr)(JT_C|TMS|TDI);
			(*jt_putcsr)(JT_C|TDI);
			jtag_tab.state=0;			// state RTI
		}

		return 0;
	}

	if (jtag_tab.state == 0) {
		(*jt_putcsr)(JT_C|TMS|TDI);
		(*jt_putcsr)(JT_C|TDI);			// CAPTURE-DR
		if (jt_mode) (*jt_putcsr)(JT_C|TDI);	// Shift DR

		ux=jtag_tab.num_devs;
		while (--ux != jtag_tab.select) (*jt_putcsr)(JT_C|TDI);	// header bits
		jtag_tab.state=1;
	}

	bf=(u_long*)buf;
	while (len > 4) {
		ux=0;
		do (*jt_putcsr)(JT_C|TDI); while (++ux < 32);		// erste TCK ist noch in CAPTURE-DR

		*bf++ =(*jt_data)(); len -=4;
	}

	ux=0;
	do (*jt_putcsr)(JT_C|TDI); while (++ux < 32);

	*bf++ =(*jt_data)();
	return 0;
}
//
//--------------------------- jtag_wr_data ----------------------------------
//
static int jtag_wr_data(
	void	*buf,
	u_int	len,		// Anzahl Byte
	int	state)	// -1: end
{
	u_long	*bf;
	u_long	din;
	u_int		dev;
	u_int		ux;
	u_char	ms;

	if (jtag_tab.state < 0) return CE_PROMPT;

	bf=(u_long*)buf; len >>=2;
	if (len == 0) return 0;

	(*jt_putcsr)(JT_C|TMS|TDI);
	(*jt_putcsr)(JT_C|TDI); (*jt_putcsr)(JT_C|TDI); dev=jtag_tab.num_devs; ms=0;
	while (dev) {
		dev--;
		if (dev != jtag_tab.select) {
			(*jt_putcsr)((dev) ? (JT_C|TDI) : (JT_C|TMS|TDI));
		} else {
			while (len) {
				len--;
				din=*bf++; ux=32;
				while (ux) {
					ux--;
					if ((dev == 0) && (len == 0) && (ux == 0)) ms=TMS;

					(*jt_putcsr)(JT_C|ms|((u_char)din &TDI)); din >>=1;
				}
			}
		}
	}

	if (state < 0) {
		(*jt_putcsr)(JT_C|TMS|TDI); (*jt_putcsr)(JT_C|TDI); jtag_tab.state=0;
	}

	return 0;
}
//
//--------------------------- jtag_data_exchange ----------------------------
//
// TDI buffer: dr_tdi[]
// TDO buffer: dr_itdo[]
// # of bits:  sc_length
//
static int jtag_data_exchange(void)	// 0: ok
{
	u_long	*bfi,*bfo;
	u_long	din;
	u_int		dev;
	u_int		len;
	u_int		ux;

	if (jtag_tab.state < 0) return CE_PROMPT;

	if (!sc_length) return 0;

	bfi=(u_long*)dr_tdi;
	bfo=(u_long*)dr_itdo;
	len =sc_length >>5;

	(*jt_putcsr)(JT_C|TMS|TDI);
	(*jt_putcsr)(JT_C|TDI);
	(*jt_putcsr)(JT_C|TDI);		// SHIFT-DR
	dev=jtag_tab.num_devs;
	while (dev) {
		dev--;
		if (dev != jtag_tab.select) {		// TDI/TDO header and tail
			if (dev == 0) {
				(*jt_putcsr)(JT_C|TMS|TDI);		// letztes device, EXIT1-DR
				break;
			}

			(*jt_putcsr)(JT_C|TDI);
			continue;
		}
//
// --- selected device
//
		while (len) {
			len--;
			din=*bfi++; ux=32;
			while (ux) {
				ux--;
				if (ux == 0) {
					*bfo++ =(*jt_data)();
					if (   (len == 0) && (dev == 0)
						 && !(sc_length &0x1F)) {			// absolut letztes Bit
						(*jt_putcsr)(JT_C|TMS |((u_char)din &TDI));	// EXIT1-DR
						break;
					}
				}

				(*jt_putcsr)(JT_C|((u_char)din &TDI)); din >>=1;
			}

		}

		if ((ux=sc_length &0x1F) != 0) {
			din=*bfi++;
			while (ux) {
				ux--;
				if (ux == 0) {
					*bfo++ =(*jt_data)() >>(32-(sc_length &0x1F));
					if (dev == 0) {
						(*jt_putcsr)(JT_C|TMS|((u_char)din &TDI));	// EXIT1-DR
						break;
					}
				}

				(*jt_putcsr)(JT_C|((u_char)din &TDI)); din >>=1;
			}
		}
	}

	(*jt_putcsr)(JT_C|TMS|TDI); (*jt_putcsr)(JT_C|TDI);	// state RTI
	return 0;
}
//
//--------------------------- read_idcodes ----------------------------------
//
static int read_idcodes(void)
{
	JTAG_TAB	*jtab;
	u_int		ux;
	u_long	tmp;

	printf("\nread JTAG chain\n\n");
	if ((jtab=jtag_reset()) == 0) {
		printf("no JTAG chain found\n");
		return CE_PROMPT;
	}

	printf("JTAG chain\n");
	printf("-----------\n"); ux=0; jtab->state=0;
	do {
		printf(" %u: %08lX", ux, tmp=jtab->devs[ux].idcode);
		if ((tmp &0x0FFFFFFFl) == 0x05024093l) {
			printf(" XC18V01  Ver: %u", (u_char)(tmp >>28));
			jtab->devs[ux].ir_len=8;
		} else {
			if ((tmp &0x0FFFFFFFl) == 0x05025093l) {
				printf(" XC18V02  Ver: %u", (u_char)(tmp >>28));
				jtab->devs[ux].ir_len=8;
			} else {
				if ((tmp &0x0FFFFFFFl) == 0x05026093l) {
					printf(" XC18V04  Ver: %u", (u_char)(tmp >>28));
					jtab->devs[ux].ir_len=8;
				} else {
					if ((tmp &0x0FFFFFFFl) == 0x00618093l) {
						printf(" XCV150   Ver: %u", (u_char)(tmp >>28));
						jtab->devs[ux].ir_len=5;
					} else {
						if ((tmp &0x0FFFFFFFl) == 0x00628093l) {
							printf(" XCV400   Ver: %u", (u_char)(tmp >>28));
							jtab->devs[ux].ir_len=5;
						} else {
							jtab->state=-1;
							printf(" unknown device");
						}
					}
				}
			}
		}

		Writeln();
	} while (++ux < jtab->num_devs);

	Writeln();
	return CE_PROMPT;
}
//
//--------------------------- jtag_instr ------------------------------------
//
static int jtag_instr(
	int	instr)	// -1: dialog
{
	int	ret;

	printf("device      %4u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if (instr >= 0) printf("instruction 0x%02X\n", instr);
	else {
		printf("instruction 0x%02X ", jt_conf->jtag_instr);
		jt_conf->jtag_instr=(u_char)Read_Hexa(jt_conf->jtag_instr, 0xFF);
		if (errno) return 0;

		instr=jt_conf->jtag_instr;
	}

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	if ((ret=jtag_instruction(instr)) < 0)
		printf("JTAG not initialized\n");
	else
		printf("instructin reg =0x%02X\n", ret);

	return CE_PROMPT;
}
//
//--------------------------- jtag_exchange ---------------------------------
//
static int jtag_exchange(void)
{
	u_long	dout;

	printf("data length    %8u ", jt_conf->jtag_dlen);
	jt_conf->jtag_dlen=(u_char)Read_Deci(jt_conf->jtag_dlen, 32);
	if (errno) return 0;

	printf("JTAG data in 0x%08lX ", jt_conf->jtag_data);
	jt_conf->jtag_data=Read_Hexa(jt_conf->jtag_data, -1);
	if (errno) return 0;

	dout=jtag_data(jt_conf->jtag_data, jt_conf->jtag_dlen);
	if (errno)
		printf("JTAG not initialized\n");
	else
		printf("data reg =0x%08lX\n", dout);

	return CE_PROMPT;
}
//
//--------------------------- jtag_prom -------------------------------------
//
static int jtag_prom(
	int	mode)	// 0: program
					// 1: verify
					// 2: read and save
{
	int		ret;
	u_long	dout;
	u_int		ux,uy,ex;
	u_long	lx;
	u_char	pt;
	u_int		data0;		// beachte Groesse dr_tdi[]
	u_long	size0;
	char		*fname;
	u_int		date, time;

	pt=0;
	read_idcodes();
	if (jtag_tab.state) return CE_PROMPT;

	printf("device    %3u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	if (mode == 2) {
		fname =(jt_conf->jtag_dev < C_JT_NM) ? jt_conf->jtag_file[jt_conf->jtag_dev] :
														jt_conf->jtag_file[C_JT_NM-1];
		printf("bin file name ");
		strcpy(fname,
				 Read_String(fname, sizeof(jt_conf->jtag_file[0])));
		if (errno) return 0;

		if ((mcs_hdl=_rtl_creat(fname, 0)) == -1) {
			perror("error creating file");
			return CEN_PROMPT;
		}
	} else {
		fname =(jt_conf->jtag_dev < C_JT_NM) ? jt_conf->jtag_file[jt_conf->jtag_dev] :
														jt_conf->jtag_file[C_JT_NM-1];
		printf("MCS file name ");
		strcpy(fname,
				 Read_String(fname, sizeof(jt_conf->jtag_file[0])));
		if (errno) return 0;

		if ((mcs_hdl=_rtl_open(fname, O_RDONLY)) == -1) {
			perror("error opening MCS file");
			return CE_PROMPT;
		}

		_dos_getftime(mcs_hdl, &date, &time);
		printf("time stamp: %02u.%02u.%02u  %2u:%02u:%02u\n",
				 date&0x1F, (date>>5)&0x0F, (date>>9)-20,
				 (time>>11)&0x1F, (time>>5)&0x3F, (time<<1)&0x3F);
		mcs_ix  =1;
		mcs_blen=1;
		if ((ret=MCS_record()) != 0) return ret;
		if ((mcs_record.type != 2) && (mcs_record.type != 4)) return CE_FORMAT;
	}

	if ((ret=jtag_instruction(IDCODE)) < 0) {
		printf("JTAG not initialized\n");
		return CE_PROMPT;
	}
//
//--- ueberpruefe ID Code, siehe xc18v0?.bsd File
//
	dout=jtag_data(-1, 32);
	if ((dout &0x0FFFFFFFL) == 0x05024093L) {			// XC18V01 1 Mbit
		data0=0x100; size0=0x20000L;
	} else {
		if ((dout &0x0FFFFFFFL) == 0x05025093L) {		// XC18V02 2 Mbit
			data0=0x200; size0=0x40000L;
		} else {
			if ((dout &0x0FFFFFFFL) == 0x05026093L) {	// XC18V04 4 Mbit
				data0=0x200;  size0=0x80000L;
			} else {
				printf("wrong IDCODE: %08lX\n", dout);
				return CE_PROMPT;
			}
		}
	}

	if (mode == 2) {
//
//--- ISP PROM auslesen und binaer speichern
//
		jtag_instruction(ISPEN);
		jtag_data(0x34, 6);
		ret=jtag_instruction(FADDR);
		if (ret != 0x11) printf("FADDR.3 %02X\n", ret);

		jtag_data(0, 16);
		jtag_instruction(FVFY1);

		ux=0;
		do dout +=(*jt_data)(); while (++ux < 50);	// mindestens 20
		ex=0; lx=0;
		do {
			printf(".");
			if ((ret=jtag_rd_data(dr_tdi, sizeof(dr_tdi))) != 0) {
				Writeln(); return ret;
			}

			if (write(mcs_hdl, dr_tdi, sizeof(dr_tdi)) != sizeof(dr_tdi)) {
				Writeln();
				perror("error writing file");
				return CEN_PROMPT;
			}

			lx +=sizeof(dr_tdi);
		} while (lx < size0);

		Writeln();
		return CE_PROMPT;

	}

	if (mode == 0) {
//
//--- ISP PROM programmieren
//
		jtag_instruction(ISPEN);
		jtag_data(0x04, 6);

		ret=jtag_instruction(FADDR);
		if (ret != 0x11) printf("FADDR.0 %02X\n", ret);
		jtag_data(1, 16);
		jtag_instruction(FERASE);

		ux=0; do dout +=(*jt_data)(); while (++ux < 1000);	// mindestens 500
//		delay(1);
		jtag_instruction(NORMRST);

		ret=jtag_instruction(BYPASS);
		if (ret != 0x01) printf("BYPASS.0 %02X\n", ret);

		jtag_instruction(ISPEN);
		jtag_data(0x04, 6);

		lx=0;
		do {
			if (!(lx &0xFFF)) { printf("."); pt=1; }
			ux=0;
			do {
				do {
					if ((ret=MCS_record()) != 0) {
						Writeln();
						if (ret == -1) ret=CE_FORMAT;

						return ret;
					}

				} while ((mcs_record.type != 0) && (mcs_record.type != 1));

				if (mcs_record.type == 1) {
					Writeln();
					printf("END record\n");
					break;
				}

				uy=0;
				do dr_tdi[ux++] =mcs_record.data[uy++];
				while (uy < mcs_record.bc);

			} while (ux < data0);

			lx +=ux;
			while (ux < data0) dr_tdi[ux++] =0xFF;

			jtag_instruction(FDATA0);
			if ((ret=jtag_wr_data(dr_tdi, data0, -1)) != 0) return ret;

			if (lx <= data0) {
				ret=jtag_instruction(FADDR);
				if (ret != 0x11) printf("FADDR.1 %02X\n", ret);

				jtag_data(0, 16);
			}

			jtag_instruction(FPGM);
			ux=0; do dout +=(*jt_data)(); while (++ux < 1000);	// mindestens 500
		} while (mcs_record.type == 0);

		ret=jtag_instruction(FADDR);
		if (ret != 0x11) printf("FADDR.2 %02X\n", ret);

		jtag_data(1, 16);
		jtag_instruction(SERASE);
		ux=0; do dout +=(*jt_data)(); while (++ux < 1000);	// mindestens 500

		jtag_instruction(NORMRST);

		ret=jtag_instruction(BYPASS);
		if (ret != 0x01) printf("BYPASS.1 %02X\n", ret);

		printf("0x%06lX byte written\n", lx);
		mcs_ix  =1;
		mcs_blen=1;
		lseek(mcs_hdl, 0, SEEK_SET);
		Writeln(); pt=0;
		printf("verify\n");
	}
//
//--- ISP PROM ruecklesen und vergleichen
//
	jtag_instruction(ISPEN);
	jtag_data(0x34, 6);
	ret=jtag_instruction(FADDR);
	if (ret != 0x11) printf("FADDR.3 %02X\n", ret);

	jtag_data(0, 16);
	jtag_instruction(FVFY1);

	ux=0;
	do dout +=(*jt_data)(); while (++ux < 50);	// mindestens 20
	ex=0; lx=0;
	while (1) {
		if ((ret=jtag_rd_data(dr_tdi, sizeof(dr_tdi))) != 0) return ret;

		printf("."); pt=1;
		ux=0;
		do {
			do {
				if ((ret=MCS_record()) != 0) {
					Writeln();
					if (ret == -1) ret=CE_FORMAT;

					return ret;
				}

				if (mcs_record.type == 1) {
					Writeln();
					printf("END record");
					return CE_PROMPT;
				}

			} while (mcs_record.type != 0);

			uy=0;
			do {
				if (dr_tdi[ux] != mcs_record.data[uy]) {
					if (pt) { Writeln(); pt=0; }
					printf("%06lX: error, exp %02X, is %02X\n",
							 lx, mcs_record.data[uy], dr_tdi[ux]);
					if (++ex >= 20) {
						return CE_PROMPT;
					}
				}

				ux++; uy++; lx++;
			} while (uy < mcs_record.bc);
		} while (ux < sizeof(dr_tdi));
	}

}
//
//--------------------------- jtag_svf --------------------------------------
//
static int jtag_svf(void)
{
	int		ret;
	u_long	dx;
	int		arg;
	u_int		i;
	u_int		ix,lx;
	char		*fname;
	int		hd;
	u_int		date, time;
	u_char	*bf;
	u_int		pt,ex;
	u_long	tlen;

	read_idcodes();
	if (jtag_tab.state) return CE_PROMPT;

	printf("device    %3u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	fname =(jt_conf->jtag_dev < C_JT_NM) ? jt_conf->jtag_svf_file[jt_conf->jtag_dev] :
													jt_conf->jtag_svf_file[C_JT_NM-1];
	printf("SVF file name ");
	strcpy(fname,
			 Read_String(fname, sizeof(jt_conf->jtag_svf_file[0])));
	if (errno) return 0;

	if ((hd=_rtl_open(fname, O_RDONLY)) == -1) {
		perror("error opening MCS file");
		return CE_PROMPT;
	}

	_dos_getftime(hd, &date, &time);
	close(hd);
	printf("time stamp: %02u.%02u.%02u  %2u:%02u:%02u\n",
			 date&0x1F, (date>>5)&0x0F, (date>>9)-20,
			 (time>>11)&0x1F, (time>>5)&0x3F, (time<<1)&0x3F);
	if ((svfdat=fopen(fname, "r")) == 0) {
		perror("error opening file");
		return CE_PROMPT;
	}

	i=0; sc_line=0;
	pt=0; ex=0;
	tlen=0;
	while (1) {
		svf_line[svf_ix=0]=0;
		if ((ret=svf_token()) <= SC_UNDEF) {
			if (ret == SC_UNDEF) { i=0; break; }

			return CEN_PROMPT;
		}

		if (ret == SC_COMMENT) {
//			printf("%s\n", svf_line);
			continue;
		}

		if (ret > SC_RUNTEST) {
			if (pt) { Writeln(); pt=0; }
			printf("illegal SVF command %d in line %u\n", ret, sc_line);
			svf_line[80]=0;
			printf("%s\n", svf_line);
			return CEN_PROMPT;
		}

		if (ret < SC_SIR) continue;

		sc_command=ret;
		svf_ix +=svf_a2bin(0, svf_line+svf_ix);

		if ((svf_line[svf_ix] != ' ') && (svf_line[svf_ix] != TAB)) { i=1; break; }

		if (sc_command == SC_RUNTEST) {
			if (svf_token() != SC_TCK) { i=2; break; }

			if (svf_token() != SC_SEMICOLON) { i=3; break; }

//	printf("runtest %lu\n", svf_value);
			jtag_runtest(svf_value);
			continue;
		}

		if (!(sc_length=(u_int)svf_value)) { i=4; break; }

		lx=(sc_length+7) /8;
		if (lx > sizeof(dr_tdi)) {
			if (pt) { Writeln(); pt=0; }
			printf("date to long %u, in line\n", lx, sc_line);
			return CEN_PROMPT;
		}

		tdo_val=0;
		if (lx <= 4) {
			while (1) {
				ret=svf_token();
				if ((ret < SC_TDI) || (ret > SC_MASK)) break;

				arg=ret;
				if ((ret=svf_token()) != SC_BRACKET_O) break;

				svf_ix +=svf_a2bin(1, svf_line+svf_ix);
				if ((ret=svf_token()) != SC_BRACKET_C) break;

				if (sc_command == SC_SIR)
					switch (arg) {
			case SC_TDI:
						lvir_tdi=(u_int)svf_value; break;

			case SC_TDO:
						tdo_val=1;
						lvir_tdo=(u_int)svf_value; break;

			case SC_SMASK:
						break;

			case SC_MASK:
						lvir_mask=(u_int)svf_value; break;
					}
				else
					switch (arg) {
			case SC_TDI:
						lvdr_tdi=svf_value; break;

			case SC_TDO:
						tdo_val=1;
						lvdr_tdo=svf_value; break;

			case SC_SMASK:
						break;

			case SC_MASK:
						lvdr_mask=svf_value; break;
					}
			}

			if (ret != SC_SEMICOLON) { i=5; break; }

			if (sc_command == SC_SIR) {
				if (lvir_tdi == IDCODE) {
					if (pt) { Writeln(); pt=0; }
					printf(" read ID code\n");
				}

				if (lvir_tdi == ISPEN) {
					if (pt) { Writeln(); pt=0; }
					printf(" programming enable\n");
				}

				if (lvir_tdi == NORMRST) {
					if (pt) { Writeln(); pt=0; }
					printf(" programming disable\n");
				}

				if (lvir_tdi == FADDR) {
					if (!(pt &0x3)) printf(".");
					if (++pt >= 4*70) { Writeln(); pt=0; }
				}

// printf("%d %5u %8lX %8lX %8lX\n",
// sc_command, sc_length, lvir_tdi, lvir_tdo, lvir_mask);
				if (jtag_tab.devs[jtag_tab.select].ir_len != sc_length) {
					if (pt) { Writeln(); pt=0; }
					printf("illegal length of instruction register\n");
					return CEN_PROMPT;
				}

				ret=jtag_instruction(lvir_tdi);
				if (ret < 0) return CEN_PROMPT;

				if (tdo_val && ((ret ^lvir_tdo) &lvir_mask)) {
					if (pt) { Writeln(); pt=0; }
					printf("line %d: IR %02X, exp: %02X/%02X, is: %02X\n",
							 sc_line, (u_int)lvir_tdi, (u_int)lvir_tdo, (u_int)lvir_mask, (u_int)ret);
					return CEN_PROMPT;
				}

				continue;
			}

// printf("%d %5u %8lX %8lX %8lX\n",
// sc_command, sc_length, lvdr_tdi, lvdr_tdo, lvdr_mask);
			dx=jtag_data(lvdr_tdi, sc_length);
			if (tdo_val && ((dx ^lvdr_tdo) &lvdr_mask)) {
				if (pt) { Writeln(); pt=0; }
				printf("illegal content of dr\n");
				return CEN_PROMPT;
			}

			continue;
		}
//
//--- DR ist Laenger als 32 Bit ---
//
		while (1) {
			ret=svf_token();
			if ((ret < SC_TDI) || (ret > SC_MASK)) break;

			switch (arg=ret) {
	case SC_TDI:
				bf=dr_tdi; break;

	case SC_TDO:
				tdo_val=1;
				bf=dr_tdo; break;

	case SC_SMASK:
				bf=0; break;

	case SC_MASK:
				bf=dr_mask; break;
			}

			if ((ret=svf_token()) != SC_BRACKET_O) break;

			ix=lx;
			while (ix) {
				if ((ret=svf_byte()) < 0) break;

				ix--;
				if (bf) bf[ix] =ret;
			}

			if (ret < 0) break;

			if ((ret=svf_token()) != SC_BRACKET_C) break;
		}

		if (ret != SC_SEMICOLON) { i=5; break; }

#if 0
printf("%d %5u\n",
sc_command, sc_length);
for (ix=0; ix < 8; ix++) printf(" %02X/%02X/%02X",
										  dr_tdi[ix], dr_tdo[ix], dr_mask[ix]);
Writeln();
#endif
		if (jtag_data_exchange()) return CEN_PROMPT;

		if (!tdo_val) continue;

		for (ix=0; ix < lx; ix++) {
			if ((dr_tdo[ix] ^dr_itdo[ix]) &dr_mask[ix]) {
				if (pt) { Writeln(); pt=0; }
				printf("%06lX: error, exp %02X, is %02X\n",
						 tlen+ix, dr_tdo[ix] &dr_mask[ix], dr_itdo[ix] &dr_mask[ix]);
				if (++ex >= 20) {
					return CE_PROMPT;
				}
			}
		}

		tlen +=lx;
	}

	if (pt) { Writeln(); pt=0; }
	printf("%u: illegal character in line %u, column %u\n", i, sc_line, svf_ix);
	svf_line[80]=0;
	printf("%s\n", svf_line);
	return CEN_PROMPT;
}
//
//--------------------------- jtag_load -------------------------------------
//
static int jtag_load(void)
{
	int	ret;
	int	tdo;

	read_idcodes();
	if (jtag_tab.state) return CE_PROMPT;

	printf("device    %3u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	jtag_instruction(ISPEN);
	jtag_data(0x34, 6);
	tdo=jtag_instruction(FADDR);
	if (tdo != 0x11) printf("FADDR.0 %02X\n", tdo);

	jtag_data(0x4000, 16);

	jtag_runtest(1);
	jtag_instruction(FDATA3);
	jtag_data(0x1F, 6);

	jtag_instruction(FPGM);
	jtag_runtest(14000);

	jtag_instruction(NORMRST);
	jtag_runtest(110000L);

	jtag_instruction(CONFIG);
	printf("was jetzt NORMRST?\n");
	return CEN_PROMPT;
}
//
//--------------------------- JTAG_menu -------------------------------------
//
static char *const jtag_txt[] ={
	"TDI", "TMS",  "TCK", "TDO", "?", "?", "?", "?",
	"ena", "auto", "?",   "?",   "?", "?", "?", "?"
};

int JTAG_menu(
	void		(*jtag_putcsr)(u_int),
	u_int		(*jtag_getcsr)(void),
	u_long	(*jtag_getdata)(void),
	JT_CONF	*jtag_conf,
	u_char	jtag_mode)
{
	u_int		ux;
	char		hgh;
	char		key;
	int		ret;
	u_long	tmp;

	jt_putcsr=jtag_putcsr;
	jt_getcsr=jtag_getcsr;
	jt_data=jtag_getdata;
	jt_conf=jtag_conf;
	jt_mode=jtag_mode;

	while (1) {
		Writeln();
		printf("JTAG csr              $%04X ..0 ", tmp=(*jtag_getcsr)());
		ux=16; hgh=0;
		while (ux--)
			if (tmp &((u_long)1L <<ux)) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("%s", jtag_txt[ux]);
			}
		lowvideo(); cprintf("\r\n");

		printf("JTAG data         $%08lX\n", (*jtag_getdata)());

		printf("read IDCODEs (reset)        ..1\n");
		printf("JTAG instruction            ..2\n");
		printf("JTAG data exchange          ..3\n");
		printf("JTAG program XC18V00 PROM   ..4\n");
		printf("JTAG verify XC18V00 PROM    ..5\n");
		printf("JTAG read and save binary   ..6\n");
		printf("JTAG load FPGA              ..7\n");
		printf("JTAG execute SVF file       ..8\n");
		printf("                              %c ",
					 jt_conf->jtag_menu);
		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
			Writeln();
			(*jtag_putcsr)(0);
			return 0;
		}

		while (1) {
			use_default=(key == TAB);
			if (use_default || (key == CR)) key=jt_conf->jtag_menu;

			if (key >= ' ') printf("%c", key);
			Writeln();
			errno=0; ret=-2;
			switch (key) {

		case '0':
				printf("JTAG ctrl   $%04X ", jt_conf->jtag_ctrl);
				jt_conf->jtag_ctrl=(u_int)Read_Hexa(jt_conf->jtag_ctrl, -1);
				if (errno) break;

				(*jtag_putcsr)(jt_conf->jtag_ctrl);
				ret=0;
				break;

		case '1':
				ret=read_idcodes();
				break;

		case '2':
				ret=jtag_instr(-1);
				break;

		case '3':
				ret=jtag_exchange();
				break;

		case '4':
		case '5':
		case '6':
				ret=jtag_prom(key-'4');
				Writeln();
				if (mcs_hdl != -1) { close(mcs_hdl); mcs_hdl=-1; }
				if (jtag_tab.state == 1) {
					jtag_rd_data(0, 0);
					printf("%02X\n", jtag_instruction(NORMRST));
					printf("%02X\n", jtag_instruction(BYPASS));
				}
				break;

		case '7':
				ret=jtag_load();
				break;

		case '8':
				ret=jtag_svf();
				if (svfdat) { fclose(svfdat); svfdat=0; }

				break;

			}

			while (kbhit()) getch();
			if (ret != -2) jt_conf->jtag_menu=key;

			if (ret <= 0) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if (key == TAB) continue;

			if (key < ' ') { Writeln(); break; }

			if (key == ' ') key=CR;
		}
	}
}

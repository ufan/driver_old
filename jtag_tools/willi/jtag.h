// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	jtag.h	-- JTAG protocol handler

//
// -------------------------- Configuration Buffer --------------------------
//
typedef struct {
	char		jtag_menu;
	char		tmp;
	u_int		jtag_ctrl;
	u_int		jtag_dev;
	u_int		jtag_instr;
	u_int		jtag_dlen;
	u_long	jtag_data;

#define C_JT_NM		2
	char		jtag_file[C_JT_NM][80];
	char		jtag_svf_file[C_JT_NM][80];
} JT_CONF;
//
//--------------------------- global function -------------------------------
//
int JTAG_menu(
	void		(*jtag_putcsr)(u_int),
	u_int		(*jtag_getcsr)(void),
	u_long	(*jtag_getdata)(void),
	JT_CONF	*jtag_conf,
	u_char	jtag_mode);

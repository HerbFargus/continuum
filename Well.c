/* 
	File Well.c:  init and ensure that we have the second graphics screen
	
	Copyright (c) 1986-89 Randall H. Wilson
*/


#include "GW.h"


#define	WNETrapNum		0x60
#define	UnImplTrapNum	0x9F

#define envSE30			7

/*	These constants are only used on a MacPlus, so there aren't probs
		with Mac II, and we reduce incompatibility with lo-mem globals
*/
#define CURPAGEOPTION	(*(int *) 0x936)		/* screen allocation */
#define	HARDSCR1	(*((char **) 0x824))		/* SCRNBASE	(or 37A700) */
#define	HARDSCR2	(HARDSCR1 - 0x8000)			/* should be screen 2   */


int Screen_Type,	/* type of screen we must deal with, mainly */
	Have_HFS,		/* TRUE if HFS is present */
	Have_WNE;		/* TRUE if WaitNextEvent available */


main()
{
	get_environment();
	
	if (Screen_Type != MACHINE_PLUS		/* 68020 or */
			|| CURPAGEOPTION < 0)		/* second time we were run */
		main_2();
	else if (MemTop < (char *) 200000L	/* on 128K Mac */
			|| BufPtr < (HARDSCR2 + SCRSIZE))
			/* something using 2nd screen space */
	{	if (ask_ramcache())			/* if they take responsibility, */
			get_second_screen();	/*    go ahead and try to run */
	}
	else
		get_second_screen();
}



get_environment()
{
	SysEnvRec envrec;
	int errorcode;
	extern char *primary_screen, *secondary_screen;
	extern BitMap secondbmap;
	char keymap[16];
	extern GrafPort screen_port;
	
	InitGraf(&thePort);				/* needed to check screen size */
	
	SysEnvirons(2, &envrec);
	if ((envrec.machineType >= env512KE && envrec.machineType <= envSE)
			|| envrec.machineType == envMac)
		Screen_Type = MACHINE_PLUS;
	else if (envrec.machineType == envSE30)
		Screen_Type = MACHINE_SE30;
	else if (screenBits.bounds.right == 512)
		Screen_Type = MACHINE_SMALL;
	else
		Screen_Type = MACHINE_2;
	Have_HFS = (envrec.machineType >= 0);
	Have_WNE = (NGetTrapAddress(WNETrapNum, ToolTrap) !=
				NGetTrapAddress(UnImplTrapNum, ToolTrap));
	
#ifdef	DEBUG	
	GetKeys(keymap);
	if (0x01 & keymap[7])			/* shift key down, force Mac+ */
		Screen_Type = MACHINE_PLUS;
	else if (0x04 & keymap[7] &&	/* option key & command key */
				0x80 & keymap[6])	/*  	simulate non-SE30 */
		Screen_Type = MACHINE_SMALL;
	else if (0x04 & keymap[7])		/* option key, simulate SE30 */
		Screen_Type = MACHINE_SE30;
	else if (0x80 & keymap[6])		/* command key, simulate Mac 2 */
		Screen_Type = MACHINE_2;
#endif
	
	primary_screen = HARDSCR1;
					/* will die if 68000 & not second graphics screen */
	secondary_screen =
		(Screen_Type == MACHINE_PLUS ? HARDSCR2
									 : NewPtr((long) SCRSIZE +
									 	2 * screenBits.rowBytes));
	if (secondary_screen == NULL)
		memory_error();
	secondbmap.baseAddr = secondary_screen;
	secondbmap.rowBytes = 64;
	SETRECT(secondbmap.bounds, 0, 0, SCRWTH, SCRHT);
}


int ask_ramcache()
{
	int dlognum, itemhit;
	DialogPtr dptr;
	
	dlognum = (MemTop < (char *) 200000L ? 9 : 8);
	InitGraf(&thePort);
	InitFonts();
	InitWindows();
	InitDialogs(NULL);
	InitCursor();
	if (!(dptr = GetNewDialog(dlognum, NULL, -1L)))
	{
		SysBeep(10);
		SysBeep(10);
		SysBeep(10);
	}
	frame_default(dptr);
	SysBeep(10);
	ModalDialog(0L, &itemhit);
	DisposDialog(dptr);
	return(dlognum == 8 && itemhit == 3);
}	


get_second_screen()
{
	char progname[100];
	int dummy;
	long dummylong;
	
	GetAppParms(progname, &dummy, &dummylong);
	Launch(-1, progname);			/* run program w/ 2nd screen */
}




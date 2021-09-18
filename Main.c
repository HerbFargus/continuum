/* 
	File Main.c:  main Gravity Well module
	
	Copyright (c) 1986-88 Randall H. Wilson
*/

#include "GW.h"

/*
char mydirnamep[] = "\p:Cont Files";
char hiscorefn[] = "\pHighscores";
char titlename[] = "\pTitle Screen";
char planetscrapfile[] = "\pPlanet_Clipboard";
char figurefile[] = "\pC Figures in Grid";
char sbarfile[] = "\pC Screen Top";
*/
char worldsfilep[80] = "\pContinuum Galaxy";


extern long score;
extern int fuel;
extern int cartooning;
extern int numships, endofgame;
extern linerec lines[NUMLINES];
extern bunkrec bunkers[NUMBUNKERS];
extern fuelrec fuels[NUMFUELS];
extern craterrec craters[];
extern int worldwidth, worldheight, worldwrap, shootslow;
extern int xstart, ystart, planetbonus, numcraters;
extern int gravx, gravy;
extern char cartoon[CARTOONSIZE*2];
extern int xlength[], ylength[], slopes2[], intfilter();


BitMap titlepagebmap,				/* holds title page image */
	secondbmap;						/* holds second screen on Mac II */

WindowPtr game_window;				/* ptr to game window on Mac II */

hi_sc_rec **hiscores;				/* points to loaded resource */
Rect hsrect = {158, 183, 308, 500};	/* hi scores rect */
controlkey controls[5];

int playerpos=-1;
int currentlevel = 0;				/* number of current planet */
int startlevel;
int hsattempt;

char indexes[150];
int planets, cartplanet;
int wfile, worldsvol;				/* the worlds file and volume */

extern int Screen_Type, Have_WNE, Have_HFS;	/* Flags wrt environment */
int inbackground = FALSE;			/* true when we've been suspended */

main_2()
{
	int next, planedit(), planet();
	
	init();
	
	next = NEXTTITLE;
	while (next != NEXTQUIT)
		switch (next)
		{
			case NEXTTITLE:
				next = dotitlepage();
				break;
			case NEXTEDIT:
				next = planedit();
				UnloadSeg(planedit);
				break;
			case NEXTPLAY:
				if (cartooning)
					do_cartoon();
				else
					game(startlevel);
				next = NEXTTITLE;
				UnloadSeg(planet);
				break;
		}
	
	FSClose(wfile);
	ExitToShell();
}



init()
{
	AppFile thefile;
	int dummy, count;
	
	MaxApplZone();
	InitGraf(&thePort);
	InitFonts();
	InitWindows();
	InitCursor();
	FlushEvents(everyEvent, 0);
	TEInit();
	InitDialogs(NULL);
/*	open_directory();	*/
	menu_init();
	
	gettitlepage();
	get_hi_scores();
	get_cartoon();
	get_sine_wave();
	get_controls();
	worldsvol = 0;
	CountAppFiles(&dummy, &count);
	if (count > 0)
	{	GetAppFiles(1, &thefile);
		if (thefile.fType == 'PLNT')
		{
			Pstrcpy(worldsfilep, thefile.fName);
			worldsvol = thefile.vRefNum;
		}
	}
	if (FSOpen(worldsfilep, worldsvol, &wfile) != noErr)
		ExitToShell();
	read_header();
	
	rotate_figures();				/* expand the figure definitions	*/
	init_sound();
}


/*
open_directory()
{
	extern int Have_HFS;
	WDPBRec paramblock;
	
	paramblock.ioNamePtr = (StringPtr) mydirnamep;
	paramblock.ioVRefNum = 0;
	paramblock.ioWDDirID = 0;
	paramblock.ioWDProcID = 'GWLL';
	if (Have_HFS && !PBOpenWD(&paramblock, FALSE))
		SetVol(NULL, paramblock.ioVRefNum);
}	*/


menu_init()
{
	InitMenus();
	SetMenuBar(GetNewMBar(1));			/* read menu bar from rsrc file */
	AddResMenu(GetMHandle(APPLEMENU), 'DRVR');	/* get desk accessories */
}



int dotitlepage()	/* returns code for next action (donext, temporarily) */
{
	WindowPtr titlewindow, eventwindow;
	WindowPeek wind;
	WindowRecord wrecord;
	EventRecord eventbuf;
	GrafPtr oldport;
	int donext, done;
	
	DrawMenuBar();
	
	titlewindow = GetNewWindow(130, &wrecord, -1L);
	SetPort(titlewindow);
	TextFont(0);							/* set to Chicago */
	
	cartooning = FALSE;
	done=FALSE;
	while(!done)
	{
		if (Have_WNE)
			WaitNextEvent(everyEvent, &eventbuf, 30L, NULL);
		else
		{	SystemTask();
			GetNextEvent(everyEvent, &eventbuf);
		}
		switch(eventbuf.what)
		{
		case mouseDown:
			switch(FindWindow(eventbuf.where, &eventwindow))
			{
			case inMenuBar:
				done = menucommand(MenuSelect(eventbuf.where), &donext);
				break;
			case inSysWindow:
				SystemClick(&eventbuf, eventwindow);
				break;
			case inContent:
				SelectWindow(titlewindow);
				GlobalToLocal(&eventbuf.where);
				if (PtInRect(eventbuf.where, &hsrect))
				{	playerpos = player(eventbuf.where.v);
					startlevel = 1;
					donext = NEXTPLAY;
					done = TRUE;
				}
			}
			break;
		case keyDown:
			if (eventbuf.modifiers & cmdKey)
				done=menucommand(MenuKey((char) eventbuf.message), &donext);
			break;
		case updateEvt:
			GetPort(&oldport);
			SetPort(titlewindow);
			BeginUpdate(titlewindow);
			drawtitlepage(&titlepagebmap, titlewindow);
			EndUpdate(titlewindow);
			SetPort(oldport);
			break;
		case app4Evt:
			if (eventbuf.message >> 24 == 0x01)
				inbackground = !((char) eventbuf.message & 0x01);
			break;
		case activateEvt:
			break;
		}
	}
	CloseWindow(titlewindow);
	while ( (wind = (WindowPeek) FrontWindow()) != NULL)
		CloseDeskAcc(wind->windowKind);
		
	return(donext);
}


drawtitlepage(frombmap, wind)
BitMap *frombmap;
WindowPtr wind;
{
	register int i, j;
	register char *name;
	register hi_sc_rec *hi;
	
	CopyBits(frombmap, &(wind->portBits), &(wind->portRect),
				&(wind->portRect), srcCopy, NULL);

	HLock(hiscores);
	hi = *hiscores;
	for (i=0; i < NUMHISCORES; i++)
		if (hi[i].thescore > 0L)
		{	j = 169 + 15*i;
			MoveTo(214, j);
			name = hi[i].thename;
			CtoPstr(name);
			DrawString(name);
			PtoCstr(name);
			drawnumber((long) hi[i].thelevel, 400, j, FALSE);
			drawnumber(hi[i].thescore, 484, j, FALSE);
		}
	HUnlock(hiscores);
	
	drawnumber(score, 101, 175, TRUE);
	drawnumber((long) fuel, 101, 230, TRUE);
	drawnumber((long) currentlevel, 90, 285, TRUE);

#ifdef	DEBUG	
	MoveTo(5, 300);
	switch(Screen_Type)
	{
	case MACHINE_PLUS:	DrawString("\pMac Plus");
						break;
	case MACHINE_2:		DrawString("\pMac II");
						break;
	case MACHINE_SMALL: DrawString("\pMac 2 Small");
						break;
	case MACHINE_SE30:	DrawString("\pMac SE30");
						break;
	}
#endif
}



int menucommand(menunum, itemnum, donext)
int menunum, itemnum, *donext;	/* returns TRUE iff done with title page */
{
	int done = FALSE;
	GrafPtr port;
	char accname[50];
	
	switch(menunum)
	{
	case APPLEMENU:
		switch(itemnum)
		{
			case 1:	gen_dialog(1);
					gen_dialog(10);
					break;
			
			case 2:	show_notes();
					break;
		
			default:GetItem(GetMHandle(APPLEMENU), itemnum, accname);
					GetPort(&port);
					OpenDeskAcc(accname);
					SetPort(port);
					break;
		}
		break;
	case FILEMENU:
		switch(itemnum)
		{
			case 2:	do_open(FALSE);
					break;
			case 4:	*donext = NEXTEDIT;
					done = TRUE;
					break;
			case 6: clear_high_scores();
					break;
			case 8:	*donext = NEXTQUIT;
					done = TRUE;
					break;
		}
		break;
	case GAMEMENU:
		switch(itemnum)
		{
		case 1:	*donext = NEXTPLAY;	/* play a game */
				playerpos = -1;		/* unknown player */
				startlevel = 1;
				done = TRUE;
				break;
		case 2:	startlevel = startatdialog();
				if(startlevel > MAXSTARTLEVEL || startlevel < 1)
					done = FALSE;
				else	
					done = TRUE;
				*donext = NEXTPLAY;
				playerpos = -1;
				break;
		case 4:	change_keys();
				break;
		case 6:	gen_dialog(4);
				break;
		case 7:	scoring();
				break;
		case 8:	gen_dialog(5);
				break;
		case 9:	if (cartplanet > 0)
				{
					cartooning = TRUE;
					*donext = NEXTPLAY;
					done = TRUE;
				}
				break;
		}
		break;
	}
	HiliteMenu(0);
	
	return(done);
}


do_open(newfile)
int newfile;
{
	static Point topleft = {82, 70};
	SFReply reply;
	
	if (newfile)
		SFPutFile(topleft,"\pName of new Galaxy file:","\p", NULL, &reply);
	else
		SFGetFile(topleft, "\pSelect a Galaxy file:", NULL,
								1, "PLNT", NULL, &reply);
	if (!reply.good)
		return;
	
	FSClose(wfile);
	
	Pstrcpy(worldsfilep, reply.fName);
	worldsvol = reply.vRefNum;
	if (newfile)
		Create(worldsfilep, worldsvol, 'GWLL', 'PLNT');
	FSOpen(worldsfilep, worldsvol, &wfile);
	if (newfile)
	{
		planets = 0;
		cartplanet = 0;
		add_planet();
	}
	else
	{
		read_header();
		currentlevel = 1;
	}
}	

read_header()
{
	SetFPos(wfile, fsFromStart, 0L);		/* read the indexes */
	if (getw(wfile) != -17)
		ExitToShell();
	planets = getw(wfile);
	cartplanet = getw(wfile);
	SetFPos(wfile, fsFromStart, 10L);
	macread(wfile, sizeof(indexes), indexes);
}


int startatdialog()
{
	register DialogPtr dptr;
	int itemnum, correct;
	int retval;
	
	dptr = GetNewDialog(2, NULL, -1L);
	frame_default(dptr);
	SelIText(dptr, 3, 0, 100);				/* select the number */
	do
	{
		correct = TRUE;
		do
			ModalDialog(intfilter, &itemnum);
		while (itemnum != 1 && itemnum != 2);
		retval = (itemnum == 2 ? 0 : 
								dlogvalue(dptr, 3, 1, planets, &correct));
		if (!correct)
			SysBeep(10);
	}
	while (!correct);
	DisposDialog(dptr);
	return(retval);
}



Rect brects[8] =
{
	{27, 20, 45, 100},
	{27, 120, 45, 200},
	{67, 70, 85, 150},
	{107, 20, 125, 100},
	{107, 120, 125, 200},
	{20, 220, 45, 300},
	{60, 220, 85, 300},
	{100, 220, 125, 300}
};
char *bstrings[8] =
{	"Turn Left", "Turn Right", "Shield", "Thrust", "Fire",
	"Continue", "Cancel", "Set Default"};
Point bstrpts[13] =
{	{23, 60}, {23, 160}, {63, 110}, {103, 60}, {103, 160},
	{38, 260}, {78, 260}, {118, 260},
	{40, 60}, {40, 160}, {80, 110}, {120, 60}, {120, 160}	};

change_keys()
{
	register int item, i;
	char *nameptr, keymap[16];
	Rect wrect;
	register WindowPtr thewindow;
	WindowRecord wrecord;
	EventRecord eventbuf;
	controlkey tempkeys[5], *thekey;
	int clickinrect, j, pressed, offset;
	unsigned char mask1;

	for (i=0; i<5; i++)
		tempkeys[i] = controls[i];
	
	SETRECT(wrect, 90, 100, 410, 240);
	thewindow = NewWindow(&wrecord, &wrect, NULL, TRUE, 
								dBoxProc, -1L, FALSE, 0L);
	SetPort(thewindow);
	TextFont(0);		/* set to Chicago */
	for (i=0; i<13; i++)
	{	if (i < 8)
		{
			if (i<5)
				FrameRect(&brects[i]);
			else
				FrameRoundRect(&brects[i], 10, 10);
			dlogstring(bstrings[i], bstrpts[i]);
		}
		else
			dlogstring(tempkeys[i-8].name, bstrpts[i]);
	}
	
	do
	{
		clickinrect = FALSE;
		while( !clickinrect)
		{	GetNextEvent(everyEvent, &eventbuf);
			if (eventbuf.what != mouseDown)
				continue;
			GlobalToLocal(&eventbuf.where);
			for(item=0; item<8; item++)
				if (PtInRect(eventbuf.where, &brects[item]))
				{
					clickinrect = TRUE;
					break;
				}
		}
		if (item<5)
			InvertRect(&brects[item]);
		else
			InvertRoundRect(&brects[item], 10, 10);
		while(StillDown())	;
		if (item >= 5)
			continue;
		thekey = &tempkeys[item];
		pressed = FALSE;
		while(!Button() && !pressed)
		{
			GetKeys(keymap);
			for(i=0; i<12; i++)
				for(mask1=1, j=0; j<8; mask1 <<= 1, j++)
					if ( mask1 & keymap[i])
					{	pressed = TRUE;
						thekey->index = i;
						thekey->mask = mask1;
						offset = i*8 + j;
					}
		}
		if (pressed)
		{	if (offset < 48 && offset != 36)
			{	GetNextEvent(keyDownMask, &eventbuf);
				thekey->name[0] = (char) eventbuf.message;
				thekey->name[1] = '\0';
			}
			else
			{	switch(offset)
				{
					case 36:	nameptr = "return";
								break;
					case 48:	nameptr = "tab";
								break;
					case 49:	nameptr = "space";
								break;
					case 50:	nameptr = "`";
								break;
					case 51:	nameptr = "backspace";
								break;
					case 52:	nameptr = "enter";
								break;
					case 55:	nameptr = " ";
								nameptr[0] = 0x11;   /* command key */
								break;
					case 56:	nameptr = "shift";
								break;
					case 58:	nameptr = "option";
								break;
					default:	nameptr = "unknown";
								break;
				}
				strcpy(thekey->name, nameptr);
			}
		}
		EraseRect(&brects[item]);
		FrameRect(&brects[item]);
		dlogstring(tempkeys[item].name, bstrpts[item+8]);
	}
	while(item < 5);
	
	if (item != 6)
	{
		for (i=0; i<5; i++)
			controls[i] = tempkeys[i];
		if (item == 7)
		{
			Handle h = GetResource('MISC', M_CONTROLS);
			move_bytes(controls, *h, sizeof(controls));
			ChangedResource(h);
		}
	}
	CloseWindow(thewindow);
}

dlogstring(thestring, where)
char *thestring;
Point where;
{
	CtoPstr(thestring);
	MoveTo(where.h - (StringWidth(thestring) >> 1), where.v);
	DrawString(thestring);
	PtoCstr(thestring);
}



drawnumber(n, x, y, drawzero)
long n;
int x, y, drawzero;
{	
	char numstr[20];
	
	NumToString(n, numstr);
	MoveTo(x - StringWidth(numstr), y);
	DrawString(numstr);
}



pascal void drawbase(thewin, itemno)
WindowPtr thewin;
int itemno;
{
	int type;
	Rect baserect;
	Handle itemh;
	BitMap bits;
	extern Bunker_Pic *bunker_defs;
		
	GetDItem(thewin, itemno, &type, &itemh, &baserect);
	baserect.right = baserect.left + BUNKHT;
	baserect.bottom = baserect.top + BUNKHT;
	
	if (itemno >= 5 && itemno <= 7)
		bits.baseAddr = (char *) bunker_defs[DIFFBUNK][itemno-5];
	else
	{
		switch(itemno)
		{
			case 2: type = GROUNDBUNK;	break;
			case 3: type = FOLLOWBUNK;	break;
			case 4: type = GENERATORBUNK; break;
			case 8: type = WALLBUNK;	break;
			default: return;	/* shouldn't happen */
		}
		bits.baseAddr = (char *) bunker_defs[type][0];
	}
	bits.rowBytes = 6;
	SETRECT(bits.bounds, 0, 0, BUNKHT, BUNKHT);
	CopyBits(&bits, &thewin->portBits,
			&bits.bounds, &baserect,
			srcOr, NULL);
}


scoring()
{
	DialogPtr dptr;
	Handle itemh;
	register int i;
	Rect baserect;
	int type, itemhit;

	dptr = GetNewDialog(14, NULL, -1L);
	for (i=2; i < 9; i++)
	{
		GetDItem(dptr, i, &type, &itemh, &baserect);
		SetDItem(dptr, i, userItem, drawbase, &baserect);
	}
	ShowWindow(dptr);
	frame_default(dptr);
	ModalDialog(NULL, &itemhit);
	DisposDialog(dptr);
}


game(startlevel)
int startlevel;
{
	score = 0;
	fuel = FUELSTART;
	numships = SHIPSTART;
	endofgame = FALSE;
	
	if (Screen_Type == MACHINE_2)
	{
		game_window = GetNewWindow(135, NULL, -1L);
		SetPort(game_window);
	}
	else
	{
		HideCursor();
		game_window = GetNewWindow(131, NULL, -1L);
	}
	open_sound();
	hide_sound();
	hsattempt = (cartplanet > 0 && startlevel == 1);
	for (currentlevel=startlevel; 
			!endofgame && currentlevel <= planets;
			currentlevel++)
	{	
		get_planet(currentlevel);
		show_sound();
		planet();
		hide_sound();
		numships++;
	}
	currentlevel--;				/* compensate for extra level after death */
	normal_screen();
	close_sound();
	if (Screen_Type != MACHINE_2)
		ShowCursor();
	FlushEvents(everyEvent, 0);

	if (hsattempt)
		new_hi_score();
	DisposeWindow(game_window);
}



do_cartoon()
{
	score = 0;
	fuel = FUELSTART;
	numships = 0;
	endofgame = FALSE;
	currentlevel = cartplanet;
	
	if (Screen_Type == MACHINE_2)
	{
		game_window = GetNewWindow(135, NULL, -1L);
		SetPort(game_window);
	}
	else
	{
		HideCursor();
		game_window = GetNewWindow(131, NULL, -1L);
	}
	
	get_planet(cartplanet);
	open_sound();
	planet();
	
	close_sound();
	normal_screen();
	
	DisposeWindow(game_window);
	if (Screen_Type != MACHINE_2)
		ShowCursor();
}



get_planet(planetnum)
int planetnum;
{
	Handle planh = NewHandle(PLANSIZE);
	
	HLock(planh);
	SetFPos(wfile, fsFromStart,
			FILEHEAD + PLANSIZE * (long) indexes[planetnum-1]);
	macread(wfile, PLANSIZE, *planh);
	unpack_planet(planh);
	DisposHandle(planh);
}


/*	Unpack_Planet:  unpacks and interprets the data in 1900 bytes of h and
		stores it in the global data structures as the current planet.
*/
unpack_planet(h)
Handle h;
{
	register linerec *line;
	register bunkrec *bunk;
	fuelrec *fuel;
	register int *ip, j, ud_and_t;
	
	ip = (int *) *h;
	
	worldwidth = *ip++;
	worldheight = *ip++;
	worldwrap = *ip++;
	shootslow = *ip++;
	xstart = *ip++ % worldwidth;
	ystart = *ip++;
	planetbonus = *ip++;
	gravx = *ip++;
	gravy = *ip++;
	numcraters = *ip++;
	
	ip = (int *) (*h + PLANHEAD);
	
	for (line=lines; line<lines+NUMLINES; line++)
	{
		line->startx = *ip++;
		line->starty = *ip++;
		line->length = *ip++;
		ud_and_t = *ip++;
		line->up_down = (char) (ud_and_t >> 8);
		line->type = ud_and_t & 7;
		line->kind = (ud_and_t & 31) >> 3;
		if (line->type == LINE_NNE || line->type == LINE_ENE)
			line->length |= 1;
		line->endx = line->startx +((xlength[line->type] * line->length)>>1);
		line->endy = line->starty + line->up_down * 
							((ylength[line->type] * line->length) >> 1);
		line->newtype = (line->up_down == L_UP ? 10-line->type : line->type);
		if (!line->type || line->endx > 4000 || line->starty > 4000)
			line->startx = 10000;
	}
	for (bunk=bunkers; bunk<bunkers+NUMBUNKERS; bunk++)
	{
		bunk->x = *ip++;
		bunk->y = *ip++;
		bunk->rot = *ip++;
		if (bunk->rot == -1)
			bunk->kind = 0;
		else
		{	bunk->kind = bunk->rot >> 8;
			bunk->rot &= 255;
		}
		for (j=0; j < 4; j++)
			((int *) bunk->ranges) [j] = *ip++;
		bunk->alive = TRUE;
		if (bunk->rot < 0 || bunk->x > 4000 || bunk->y > 4000)
			bunk->x = 10000;
	}
	for (fuel=fuels; fuel<fuels+NUMFUELS; fuel++)
	{
		fuel->x = *ip++;
		fuel->y = *ip++;
		fuel->currentfig = fuel->figcount = 1;
		fuel->alive = TRUE;
		if (fuel->x > 4000 || fuel->y > 4000)
			fuel->x = 10000;
	}
	fuels[NUMFUELS-1].x = 20000;
	move_bytes(ip, craters, sizeof(craterrec)*NUMPRECRATS);
}


/*					The old version, replaced by the above
get_planet(planet)
int planet;
{
	long offset;
	register linerec *line;
	register bunkrec *bunk;
	fuelrec *fuel;
	int f, ud_and_t;
	
	if (planet >= 0)
	{
		offset = FILEHEAD + PLANSIZE * (long) indexes[planet-1];
		f = wfile;
	}
	else
	{
		offset = 0;
		FSOpen(planetscrapfile, 0, &f);
	}
	SetFPos(f, fsFromStart, offset);
	
	worldwidth = getw(f);
	worldheight = getw(f);
	worldwrap = getw(f);
	shootslow = getw(f);
	xstart = getw(f) % worldwidth;
	ystart = getw(f);
	planetbonus = getw(f);
	gravx = getw(f);
	gravy = getw(f);
	numcraters = getw(f);
	
	SetFPos(f, fsFromStart, offset+PLANHEAD);
	
	for (line=lines; line<lines+NUMLINES; line++)
	{
		macread(f, sizeof(int)*3, line);
		ud_and_t = getw(f);
		line->up_down = (char) (ud_and_t >> 8);
		line->type = ud_and_t & 7;
		line->kind = (ud_and_t & 31) >> 3;
		if (line->type == LINE_NNE || line->type == LINE_ENE)
			line->length |= 1;
		line->endx = line->startx +((xlength[line->type] * line->length)>>1);
		line->endy = line->starty + line->up_down * 
							((ylength[line->type] * line->length) >> 1);
		line->newtype = (line->up_down == L_UP ? 10-line->type : line->type);
		if (!line->type || line->endx > 4000 || line->starty > 4000)
			line->startx = 10000;
	}
	for (bunk=bunkers; bunk<bunkers+NUMBUNKERS; bunk++)
	{
		macread(f, sizeof(int)*3, bunk);
		if (bunk->rot == -1)
			bunk->kind = 0;
		else
		{	bunk->kind = bunk->rot >> 8;
			bunk->rot &= 255;
		}
		macread(f, sizeof(rangerec)*2, bunk->ranges);
		bunk->alive = TRUE;
		if (bunk->rot < 0 || bunk->x > 4000 || bunk->y > 4000)
			bunk->x = 10000;
	}
	for (fuel=fuels; fuel<fuels+NUMFUELS; fuel++)
	{
		macread(f, sizeof(int)*2, fuel);
		fuel->currentfig = fuel->figcount = 1;
		fuel->alive = TRUE;
		if (fuel->x > 4000 || fuel->y > 4000)
			fuel->x = 10000;
	}
	fuels[NUMFUELS-1].x = 20000;
	macread(f, sizeof(craterrec)*NUMPRECRATS, craters);
	if (planet < 0)
		FSClose(f);
}	*/



int player(y)
int y;
{
	register int pos;
	Rect inv_rect;
	
	pos = (y - 158) / 15;
	SETRECT(inv_rect, 185, 158 + pos*15, 500, 173 + pos*15);
	InvertRect(&inv_rect);
	Delay(30L, &inv_rect);
	
	if ((*hiscores)[pos].thename[0] == '\0' || pos >= NUMHISCORES)
		pos = -1;
	return(pos);
}



get_cartoon()
{
	Handle h;
/*	int f;
	
	FSOpen("\pCartoon", 0, &f);
	macread(f, sizeof(cartoon), cartoon);
	FSClose(f);
	h = GetResource('MISC', M_CARTOON);
	SetHandleSize(h, (long) sizeof(cartoon));
	move_bytes(cartoon, *h, sizeof(cartoon));
	ChangedResource(h);
*/
	h = GetResource('MISC', M_CARTOON);
	move_bytes(*h, cartoon, sizeof(cartoon));
	ReleaseResource(h);
}


get_sine_wave()
{
	extern char sine_wave[256];
	Handle h = GetResource('MISC', M_SINEWAVE);
	move_bytes(*h, sine_wave, sizeof(sine_wave));
	ReleaseResource(h);
}


get_controls()
{
	move_bytes(*GetResource('MISC', M_CONTROLS), controls, sizeof(controls));
}



get_hi_scores()
{
	hiscores = (hi_sc_rec **) GetResource('MISC', M_HISCORES);
}


clear_high_scores()
{
	register hi_sc_rec *p;
	
	if (gen_dialog(28) == 2)
	{
		HLock(hiscores);
		for (p=(*hiscores); p < (*hiscores)+NUMHISCORES; p++)
		{	p->thescore = 0L;
			p->thelevel = 0;
			p->thename[0] = '\0';
		}
		HUnlock(hiscores);
		ChangedResource(hiscores);			/* make sure they're saved */
		InvalRect(&hsrect);
	}
}


new_hi_score()
{
	Rect dummyrect;
	DialogPtr dptr;
	int dummy, done;
	Handle itemhandle;
	register int i;
	char playername[260];
	register hi_sc_rec *hi;
	
	HLock(hiscores);
	hi = *hiscores;
	
	if (score <= hi[NUMHISCORES-1].thescore)
		goto unlock_and_leave;
	
	if (playerpos < 0)					/* new player	*/
	{
		dptr = GetNewDialog(3, NULL, -1L);
		frame_default(dptr);
		for(done = 0; done != 1; )
			ModalDialog(NULL, &done);
		GetDItem(dptr, 3, &dummy, &itemhandle, &dummyrect);
		GetIText(itemhandle, playername);
		DisposDialog(dptr);
		PtoCstr(playername);
		if (playername[0] == '\0')
			goto unlock_and_leave;
		for(i=0; i < NUMHISCORES-1 && 
					mystrcmp(playername, hi[i].thename); i++)
			;
	}
	else
	{	i = playerpos;
		strcpy(playername, hi[i].thename);
	}
	if (score <= hi[i].thescore)
		goto unlock_and_leave;			/* sorry, not hi enough */
	
	while (i > 0 && score > hi[i-1].thescore)
	{	hi[i] = hi[i-1];
		i--;
	}
	strncpy(hi[i].thename, playername, 19);
	hi[i].thename[19] = '\0';			/* ensure small enough	*/
	hi[i].thescore = score;
	hi[i].thelevel = currentlevel;
	ChangedResource(hiscores);			/* make sure they're saved */
	
unlock_and_leave:						/* oooooooh, a goto! */
	HUnlock(hiscores);
}




#define	TITLESIZE	12000

gettitlepage()
{
	Handle tpagehandle;
/*	int f;
	long length;
	
	if (!(tpagehandle = NewHandle((long) TITLESIZE)))
		memory_error();
		
	if ( FSOpen(titlename, 0, &f))
		ExitToShell();
	
	SetFPos(f, fsFromStart, 512L);
	length = macread(f, TITLESIZE, *tpagehandle);
	FSClose(f);
	
	SetHandleSize(tpagehandle, length);
	AddResource(tpagehandle, 'MISC', M_TITLEPAGE, NULL);
*/	
	tpagehandle = GetResource('MISC', M_TITLEPAGE);
	expandtitlepage(tpagehandle, &titlepagebmap, SCRHT);
	ReleaseResource(tpagehandle);
}


expandtitlepage(from, bitmap, lines)
Handle from;
BitMap *bitmap;
int lines;
{
	register char *dest;
	char *dataptr, destbuf[72], *dbufptr;
	register int i, j;
	
	if (!(bitmap->baseAddr = NewPtr((long) lines * 64)))
		memory_error();
	bitmap->rowBytes = 64;
	SETRECT(bitmap->bounds, 0, 0, 512, lines);

	dest = bitmap->baseAddr;
	dataptr = *from;
	for (i=0; i<lines; i++)
	{	
		dbufptr = destbuf;
		UnpackBits(&dataptr, &dbufptr, 72);
		for (j=0; j<64; j++)
			*dest++ = destbuf[j];
	}
}



init_sound()
{
	register int i;
	extern char expl_rands[], thru_rands[], hiss_rands[];
	
	for(i=0; i<128; i++)
		expl_rands[i] = (char) EXPL_LO_PER + rint(EXPL_ADD_PER);
	for(i=0; i<128; i++)
		thru_rands[i] = (char) THRU_LO_AMP + rint(THRU_ADD_AMP);
	for (i=0; i<256; i++)
		hiss_rands[i] = (char) rint(40) + 4;
		
	/* the sine_wave is read in from a file */
}




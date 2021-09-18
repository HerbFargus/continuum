/*
	Play.c:  main brains for Gravity Well for the Macintosh

	Copyright (c) 1986-88 Randall H. Wilson
*/ 


#include <ctype.h>
#include "GW.h"
#include "Macros.h"

extern linerec lines[NUMLINES];
extern bunkrec bunkers[NUMBUNKERS];
extern fuelrec fuels[NUMFUELS];
extern shardrec shards[NUMSHARDS];
extern craterrec craters[NUMCRATERS];
extern int numcraters;
extern Ship_Pic *ship_defs, *ship_masks;
extern int shield_def[];
extern int currentlevel;
extern linerec *kindptrs[];

extern int sparksalive;
extern int worldwidth, worldheight;
extern int worldwrap, xstart, ystart;
extern int planetbonus, gravx, gravy;
extern int Screen_Type;
extern BitMap secondbmap;
extern WindowPtr game_window;
	


/* x-direction thrust provided in each direction (y, too, with some work) */
int thrustx[32]={0, 9, 18, 27, 34, 40, 44, 47, 48, 47, 44, 40, 34, 27, 18, 9,
				 0,-9,-18,-27,-34,-40,-44,-47,-48,-47,-44,-40,-34,-27,-18,-9};

/* x-direction amount to add to shots when fired */
int shotvecs[32]={0, 14, 27, 40, 51, 60, 67, 71, 
				72, 71, 67, 60, 51, 40, 27, 14,
				0, -14, -27, -40, -51, -60, -67, -71, 
				-72, -71, -67, -60, -51, -40, -27, -14};

int slopes2[LINE_E+1]={0, 0, 4, 2, 1, 0};	/* slopes  of lines * 2	*/
int slopeslshift2[LINE_E+1]={0, 0, 2, 1, 0, 0};
								 /* amount to shift right for above	*/
int ylength[LINE_E+1]={0, 2, 2, 2, 1, 0};	/* ylength of lines * 2	*/
int xlength[LINE_E+1]={0, 0, 1, 2, 2, 2};	/* xlength of lines * 2	*/

int tickwait = 0;					/* for speed evaluation */

int cartooning;
unsigned int cartooncount;
char *cartoonptr;
char cartoon[CARTOONSIZE*2];

GrafPort screen_port;				/* used by MACHINE_SMALL screen type */
char *front_screen, *back_screen,	/* front and back screen pointers	*/
	*primary_screen, *secondary_screen;/* 1st and 2nd screens on this machine */
int showing_primary;				/* true if primary screen is in front */
char *change_screen = (char *) 0xeffffe;	/* used for swapping screens */
long backgr1 = 0xAAAAAAAAL,
	 backgr2 = 0x55555555L;			/* background pattern (global)		*/
long background[2];					/* pattern (local)					*/
char *sbarptr;						/* holds address of sbar background	*/
int fuelold;						/* # scrs that need new fuel count	*/

int screenx, screeny;				/* global x,y of shown screen		*/
int screenr, screenb;				/* right and bottom edges of screen */
int on_right_side;					/* true within SCRWTH of right side	*/
int flaming, thrusting;				/* true when thrusting				*/
int shielding;						/* true when shielding				*/
int refueling;						/* true when shielding close to fuel*/
int bouncing;						/* true when bounced last cycle		*/
int dead_count;						/* count for dead time				*/
int shipx, shipy;					/* screen x,y of ship center		*/
int globalx, globaly;				/* global x,y of ship center		*/
int xslow, yslow;					/* 0-255 to slow the ship			*/
int shiprot;						/* direction ship is facing			*/
int dx, dy;							/* velocity of ship (*256)			*/
char *curmessage;					/* current message being shown		*/

long score;							/* current score					*/
int fuel;							/* and fuel							*/
int numships;						/* number of player ships			*/
int endofplanet, endofgame;			/* boolean: self-explanatory		*/
int missioncomplete;				/* set true when last bunker dead	*/
int micocycles;						/* count to mico sound after above	*/
int bonuscount;						/* cycles until next bonus decrement*/

shotrec shipshots[NUMBULLETS];		/* array of ship's shots			*/
shotrec bunkshots[NUMSHOTS];		/* array of bunker shots			*/



planet()
{
	init_planet();
	
	while( !endofplanet && !endofgame)
	{
		move_and_display();
		wait_for_VR();
		swap_screens();
	}
	if (endofplanet)
	{
		score_plus(planetbonus);
		crackle();
		if (dead_count)		/* patch to deal with death as planet goes */
		{
			fuel = FUELSTART;
			numships--;
		}
	}
		
	clear_sound();
}



init_planet()
{
	register int i;
	register linerec *line;
	register bunkrec *b;
	register fuelrec *f;
	
	init_walls();
	
	endofplanet = FALSE;
	missioncomplete = bunkers[0].rot < 0;	/* if no bunks, done already */
	micocycles = MICODELAY;
	cartoonptr = cartoon;
	cartooncount = (unsigned) *cartoonptr++;
	bonuscount = 10;
	
	for(b=bunkers; b < bunkers+NUMBUNKERS; b++)
	{
		b->alive = TRUE;
		if (b->kind >= BUNKROTKINDS)
		{
			b->rot = rint(BUNKFRAMES);
			b->rotcount = rint(BUNKFCYCLES);
		}
		if (b->kind == DIFFBUNK && (b->rot & 3) == 2)
			b->rotcount = 3;			/* hard to kill */
	}
	for(f=fuels; f < fuels+NUMFUELS; f++)
	{
		f->alive = TRUE;
		f->currentfig = rint(FUELFRAMES);
		f->figcount = rint(FUELFCYCLES);
	}
	for(i=0; i<NUMBULLETS; i++)
		shipshots[i].lifecount = 0;
	init_gravity();
	init_ship();
}



init_ship()
{
	register int i;
	
	init_screens();			/* put primary screen in front */
	
	curmessage = (cartooning ? "AUTOPILOT" : NULL);
	fuel_minus(0);					/* set fuel messages, if any */
	new_sbar();
	dead_count = 0;
	
	globalx = xstart;				/* hacky, but all is straightened out */
	globaly = ystart;				/* by first call to move_ship */
	shipx = SCRWTH / 2;
	shipy = (TOPMARG + BOTMARG) / 2;
	on_right_side = FALSE;
	screenx = globalx - shipx;
	screeny = globaly - shipy;
	shiprot = cartooning ? 16 : 0;
	dx=dy=0;
	sparksalive = 0;
	shielding = thrusting = FALSE;
	for(i=0; i<NUMSHOTS; i++)
		bunkshots[i].lifecount = 0;
	for(i=0; i<NUMSHARDS; i++)
		shards[i].lifecount = 0;
}



move_and_display()
{
	if (missioncomplete && !--micocycles)
		endofplanet = TRUE;
	
	if (!--bonuscount)
	{	if ((planetbonus -= 10) < 0)
			planetbonus = 0;
		bonuscount = 10;
		write_bonus();
	}
	if (dead_count && !--dead_count)
		if( numships--)
		{	fuel = FUELSTART;
			init_ship();
		}
		else
		{	endofgame = TRUE;	
			return;
		}
	
	if (!dead_count)
	{
		ship_control();
		move_ship();
	}
	
	view_clear(back_screen);
	
	do_fuels();
	draw_craters();
	
	if (!dead_count)
		gray_figure(shipx-(SCENTER-8), shipy-(SCENTER-5),
						ship_masks[shiprot], SHIPHT);
	
	white_terrain();
	black_terrain(L_GHOST);
	
	if(!dead_count)
		erase_figure(shipx-SCENTER, shipy-SCENTER, ship_masks[shiprot], SHIPHT);
	
	check_for_bounce();
	
	black_terrain(L_NORMAL);
	do_bunkers();
	if (!shielding)
		move_bullets();
	
	if(!dead_count)
	{
		if (check_figure(shipx-SCENTER, shipy-SCENTER,
						ship_masks[shiprot], SHIPHT))
			kill_ship();
		shift_figure(shipx-(SCENTER-8), shipy-(SCENTER-5),
						ship_masks[shiprot], SHIPHT);
		full_figure(shipx-SCENTER, shipy-SCENTER,
					ship_defs[shiprot], ship_masks[shiprot], SHIPHT);
	}
	move_shipshots();
	if (shielding) 
	{	move_bullets();
		erase_figure(shipx-SCENTER, shipy-SCENTER, shield_def, SHIPHT);
	}
	if (!dead_count && flaming)
		flame_on(shipx, shipy, shiprot);
	draw_explosions();
	do_strafes();
	update_sbar();
#ifdef DEBUG
	writeint(160, 0, tickwait, back_screen);
	writeint(160, 0, tickwait, front_screen);
#endif
}


int unbouncex, unbouncey;		/* positions where last didn't bounce	*/
					/* should bounce toward that point at all times.	*/
check_for_bounce()
{
	black_terrain(L_BOUNCE);
	if(!dead_count)
		if(check_figure(shipx-SCENTER, shipy-SCENTER, ship_masks[shiprot], SHIPHT))
		{
			bounce_ship();
			bouncing = TRUE;
			erase_figure(shipx-SCENTER, shipy-SCENTER,
						ship_masks[shiprot], SHIPHT);
		}
		else
		{
			bouncing = FALSE;
			unbouncex = globalx;
			unbouncey = globaly;
		}
}

static int bounce_vecs[] =
{	0, 18, 34, 44, 48, 44, 34, 18, 0, -18, -34, -44, -48, -44, -34, -18, 0};
bounce_ship()
{
	/*	see bounce_shot for vector theory	*/
	int x1, y1, xkick, ykick, norm;
	register long dot;
	long d, dist;
	Point ship;
	extern long pt2line();
	register linerec *line;
	linerec *closest = NULL;
	
	ship.h = globalx;
	ship.v = globaly;
	dist = 1000;
	for(line = kindptrs[L_BOUNCE]; line; line = line->next)
		if((d = pt2line(&ship, line)) < dist)
		{
			dist = d;
			closest = line;
		}
	if (closest == NULL)
		return;
	norm = getstrafedir(closest, unbouncex, unbouncey);
	x1 = bounce_vecs[norm];
	y1 = bounce_vecs[(norm+12)&15];
	dot = ((long) dx) * (long) x1 + ((long) dy) * (long) y1;
	if (dot < 256*64L)
	{
		if (dot < 0)
			dot = -dot;
		if (dot < 10*256L)
			dot = 10*256L;
		xkick = (x1 * dot) / (24*48L);
		ykick = (y1 * dot) / (24*48L);
		dx += xkick;
		dy += ykick;
	}
}


kill_ship()
/* destroy the ship and do all necessary explosions, etc. */
{
	register bunkrec *bp;
	
	dead_count = DEAD_TIME;
	flaming = thrusting = refueling = shielding = FALSE;
	for(bp=bunkers; bp->rot >= 0; bp++)
		if (bp->alive &&
			xyindist(bp->x - globalx, bp->y - globaly, SKILLBRADIUS) &&
			(legal_angle(bp->rot, bp->x, bp->y, globalx, globaly) ||
									bp->kind >= BUNKROTKINDS) )
		{
			kill_bunk(bp);
			break;
		}
	start_death();
}


kill_bunk(bp)
register bunkrec *bp;
{
	static int	kindscores[5] = {100, 0, 100, 400, 500},
				diffscores[4] = {10, 200, 300, 200};
	bp->alive = FALSE;
	if (bp->kind >= BUNKROTKINDS)
	{
		craters[numcraters].x = bp->x;
		craters[numcraters].y = bp->y;
		numcraters++;
	}
	if (bp->kind == GENERATORBUNK)
		init_gravity();
	score_plus(bp->kind == DIFFBUNK ?	diffscores[bp->rot & 3] :
										kindscores[bp->kind]);
	start_explosion(bp->x, bp->y, bp->rot, bp->kind);
	start_sound(EXP1_SOUND);
	missioncomplete = TRUE;
	for(bp=bunkers; bp->rot >= 0; bp++)
		if (bp->alive && bp->kind != GENERATORBUNK)
			missioncomplete = FALSE;
	if (missioncomplete)
	{
		curmessage = "MISSION COMPLETE";
		new_sbar();
	}
}



move_ship()
{
	xslow += dx;					/* to slow down (and make smooth) */
	shipx += xslow >> 8;
	xslow &= 255;
	yslow += dy;
	shipy += yslow >> 8;
	yslow &= 255;
	
	contain_ship();
}

contain_ship()
/* hold ship inside of planet; if outside of screen, move screen rect */
{
	if (shipx < LEFTMARG)
	{	screenx += shipx - LEFTMARG;
		shipx = LEFTMARG;
	}
	else if (shipx > RIGHTMARG)
	{	screenx += shipx - RIGHTMARG;
		shipx = RIGHTMARG;
	}
	if (!worldwrap && screenx < 0)
	{	shipx += screenx;
		screenx = 0;
	}
	if (!worldwrap && screenx > worldwidth - SCRWTH)
	{	shipx += screenx - (worldwidth - SCRWTH);
		screenx = worldwidth - SCRWTH;
	}
	
	if(screenx >= worldwidth)
		screenx -= worldwidth;
	else if(screenx < 0)
		screenx += worldwidth;
		
	if (shipy < TOPMARG)
	{	screeny += shipy - TOPMARG;
		shipy = TOPMARG;
	}
	else if (shipy > BOTMARG)
	{	screeny += shipy - BOTMARG;
		shipy = BOTMARG;
	}
	if (screeny < 0)
	{	shipy += screeny;
		screeny = 0;
	}
	else if (screeny > worldheight - VIEWHT)
	{	shipy += screeny - (worldheight - VIEWHT);
		screeny = worldheight - VIEWHT;
	}
	
	screenr = screenx + SCRWTH;
	screenb = screeny + VIEWHT;
	globalx = screenx + shipx;
	if (globalx > worldwidth)
		globalx -= worldwidth;
	globaly = screeny + shipy;
	
	on_right_side = screenx > worldwidth - SCRWTH;
		
	if (shipy < SHIPHT || shipy > VIEWHT-SHIPHT)
	{
		dy = 0;
		shipy = (shipy < SHIPHT ? SHIPHT : VIEWHT - SHIPHT);
		contain_ship();  /* this recursion only goes one level deep */
	}
	if (shipx < SHIPHT || shipx > SCRWTH-SHIPHT)
	{
		dx = 0;
		shipx = (shipx < SHIPHT ? SHIPHT : SCRWTH - SHIPHT);
		contain_ship();
	}
}



ship_control()
{
	register int pressed, xdif, ydif, i;
	static int flame_blink = 4;
	static int firing = FALSE;
	register shotrec *sp;
	register fuelrec *fp;
	int xgravity, ygravity;
	
	if (cartooning)
		pressed = read_cartoon();
	else
		pressed = read_keyboard();
		
	if(pressed & KEY_LEFT)
		shiprot = (shiprot -1) & 31;
	if(pressed & KEY_RIGHT)
		shiprot = (shiprot +1) & 31;
	
	flaming = FALSE;
	if( (pressed & KEY_THRUST) && fuel)
	{	dx += (bouncing ? 1 : 2) * thrustx[shiprot];
		dy += (bouncing ? 1 : 2) * thrustx[(shiprot+24) & 31];
					/* if bouncing, make weaker to avoid flying through */
		if (--flame_blink)
			flaming = TRUE;
		else
			flame_blink = 4;
		thrusting = TRUE;
		fuel_minus(FUELBURN);
		start_sound(THRU_SOUND);
	}
	else
		thrusting = FALSE;
	
	dx -= (dx >> 6) + (dx > 0 ? 1 : 0);					/* friction	*/
	dy -= (dy >> 6) + (dy > 0 && !cartooning ? 1 : 0);
		/* the cartoon was designed for an older, wrong friction model */
	
	if (!bouncing)		/* to keep from running through bouncing walls */
	{
		gravity_vector(globalx, globaly, &xgravity, &ygravity);
		dx += xgravity;
		dy += ygravity;			/* GRAVITY !!!!!!!!!! */
	}
		
	if ( (pressed & KEY_SHIELD) && fuel)
	{	shielding = TRUE;
		start_sound(SHLD_SOUND);
		fuel_minus(FUELSHIELD);
		refueling = FALSE;
		for(fp=fuels; fp->x < 10000; fp++)
		{
			xdif = globalx - fp->x;
			ydif = globaly - fp->y;
			if (fp->alive && xyindist(xdif, ydif, FRADIUS))
			{	
				fp->alive = FALSE;
				fp->currentfig = FUELFRAMES;
				fuel_minus(-FUELGAIN);	/* wow, a kludge! */
				score_plus(SCOREFUEL);
				start_sound(FUEL_SOUND);
			}
		}
	}
	else
		shielding = FALSE;
								/* check for fire */
	if(pressed & KEY_SHOOT)
	{	if(!firing)
		{	firing = TRUE;
			for(i=0,sp=shipshots; i<NUMBULLETS && sp->lifecount; i++,sp++)
				;
			if(i<NUMBULLETS && !shielding)
			{	int yrot = (shiprot + 24) & 31;
				sp->h = shotvecs[shiprot] + (dx >> 5);
				sp->v = shotvecs[yrot] + (dy >> 5);
				sp->x8 = (globalx << 3);
				sp->y8 = (globaly << 3);
				sp->lifecount = SHOTLEN;
				sp->btime = 0;
				set_life(sp, NULL);
				if (sp->lifecount > 0)
				{
					sp->x8 += shotvecs[shiprot];
					sp->y8 += shotvecs[yrot];
					sp->lifecount--;
				}
				if (sp->lifecount == 0)
					bounce_shot(sp);
				start_sound(FIRE_SOUND);
			}
		}
	}
	else
		firing = FALSE;
}


static gravrec gravitypoints[NUMGENS];
static int numgravpoints;

/*
	note: simple circular x-dir gravity is given by function:
				((dx * gravstr) << 8) / (dx*dx + dy*dy)
*/

init_gravity()
{
	register int i;
	register bunkrec *bunk;
	
	i = 0;
	for (bunk=bunkers; bunk->rot >= 0; bunk++)
		if (bunk->alive && bunk->kind == GENERATORBUNK)
		{
			gravitypoints[i].x = bunk->x;
			gravitypoints[i].y = bunk->y;
			gravitypoints[i].str = - bunk->ranges[0].low;
			i++;
		}
	numgravpoints = i;	
}

gravity_vector(thex, they, xforce, yforce)
int thex, they;
int *xforce, *yforce;
{
	register gravrec *g, *gend = gravitypoints + numgravpoints;
	register long dx, dy, ww2;
	
	*xforce = gravx;
	*yforce = gravy;
	ww2 = worldwidth >> 1;
	for (g = gravitypoints; g < gend; g++)
	{
		dx = thex - g->x;
		if (worldwrap)
			if (dx > ww2)
				dx -= worldwidth;
			else if (dx < -ww2)
				dx += worldwidth;
		dy = they - g->y;
		asm
		{
				move.w	dx, D1
				muls	dx, D1
				move.w	dy, D0
				muls	dy, D0
				add.l	D0, D1
				muls	OFFSET(gravrec, str)(g), dx
				muls	OFFSET(gravrec, str)(g), dy
				asr.l	#4, D1		/* put into proper magnitude */
				asl.l	#4, dx
				asl.l	#4, dy
				bra.s	@enter
				
		@loop	asr.l	#4, D1
				asr.l	#4, dx
				asr.l	#4, dy
		@enter	cmp.l	#0x7FFF, D1
				bgt.s	@loop
				
				cmp.w	#8, D1
				blt.s	@leave
				
				divs	D1, dx
				move.l	xforce(A6), A0
				add.w	dx, (A0)
				divs	D1, dy
				move.l	yforce(A6), A0
				add.w	dy, (A0)
		@leave		
		}
	}
}



int read_cartoon()
{
	EventRecord eventbuf;
	
	if (GetNextEvent(mDownMask | keyDownMask, &eventbuf))
		endofgame = TRUE;					/* quit cartoon on mouse or key event */
		
	if (cartoonptr < cartoon + sizeof(cartoon))
		if (--cartooncount)
			return( (int) *cartoonptr);
		else 
		{	cartooncount = *++cartoonptr;
			return( (int) *++cartoonptr);
		}
	else
		return(0);
}

int read_keyboard()
{
	static long lasttime=0;
	register int i, keymask, keys;
	extern controlkey controls[];
	char keymap[16];
	EventRecord eventbuf;
	
	if (TickCount() > lasttime + 120)
	{		/* every 2 seconds, get event to keep screensavers away */
		lasttime = TickCount();
		GetNextEvent(keyDownMask, &eventbuf);
		ObscureCursor();			/* keep MacII from flickering */
	}
	GetKeys(keymap);
	while (0x02 & keymap[7])
	{
		clear_sound();				/* pause while caps lock is down */
		GetKeys(keymap);
	}
	
	if (0x80 & keymap[6])			/* if command key down */
	{
		if (0x10 & keymap[1])
			endofgame = TRUE;		/* quit game on ctrl-Q	*/
		if (0x01 & keymap[0])
			kill_ship();			/* abort ship on ctrl-A */
		if (0x01 & keymap[4])
			do_up_planet();			/* next planet on ctrl-U */
		if (0x40 & keymap[1])
			do_extra_ship();		/* extra ship on ctrl-E	*/
	}
	if (0x40 & keymap[5])			/* if 'M' is down, give a map */
		map_planet(TRUE);
		
	keys = 0;
	for (keymask=1, i=0; i < 5; keymask <<= 1, i++)
		if (controls[i].mask & keymap[controls[i].index])
			keys |= keymask;

	return(keys);
}

do_up_planet()
{
	extern int hsattempt;
	
	if (!hsattempt || y_n_dialog(12))
	{	
		endofplanet = TRUE;
		hsattempt = FALSE;
	}
}

do_extra_ship()
{
	extern int hsattempt;
	
	if (!hsattempt || y_n_dialog(11))
	{	
		numships++;
		hsattempt = FALSE;
		new_sbar();
	}
}


/*  Y_N_DIALOG:  puts up the given dialog and returns TRUE iff they
		pressed the "yes" button.
*/
int y_n_dialog(dlognum)
int dlognum;
{
	DialogPtr dptr;
	int hit;
	
	normal_screen();
	hide_sound();
	if (Screen_Type != MACHINE_2)
		ShowCursor();
	dptr = GetNewDialog(dlognum, NULL, -1L);
	frame_default(dptr);
	ModalDialog(NULL, &hit);
	DisposDialog(dptr);
	if (Screen_Type != MACHINE_2)
		HideCursor();
	show_sound();
	return(hit == 2);
}



move_shipshots()
{
	register shotrec *sp;
	shotrec *end;
	register bunkrec *bp;
	register int left, right, top, bot;
	int shoty, shotx;
	
	end = shipshots + NUMBULLETS;
	for (sp=shipshots; sp < end; sp++)
		if (sp->lifecount)
		{	
			move_shot(sp);
			left = sp->x - BRADIUS;
			right = sp->x + BRADIUS;
			top = sp->y - BRADIUS;
			bot = sp->y + BRADIUS;
			for (bp=bunkers; bp->x < left; bp++)
				;
			for (; bp->x < right; bp++)
				if (bp->alive && bp->y < bot && bp->y > top &&
					xyindistance(bp->x - sp->x, bp->y - sp->y, BRADIUS) &&
					(bp->kind >= BUNKROTKINDS ||
					 legal_angle(bp->rot, bp->x, bp->y,
							sp->x - (sp->h >> 3), sp->y - (sp->v >> 3))) )
				{
					sp->lifecount = sp->btime = 0;
					sp->strafedir = -1;
					if (bp->kind == DIFFBUNK &&
						(bp->rot & 3) == 2 &&
						--bp->rotcount > 0)
							break;		/* hardy bunker still alive */
					kill_bunk(bp);
					break;
				}
			if (globalx > left && globalx < right &&
				globaly > top  && globaly < bot && 
				xyindist(sp->x - globalx, sp->y - globaly, SCENTER) &&
				!dead_count)
			{
				shielding = TRUE;
				start_sound(SHLD_SOUND);
				sp->lifecount = sp->btime = 0;
				sp->strafedir = -1;
				continue;
			}
			if (sp->lifecount == 0 && sp->btime > 0)
			{
				backup_shot(sp);
				bounce_shot(sp);
			}
			shoty = sp->y - screeny - 1;
			if (shoty >= 0 && shoty < VIEWHT-3)
			{
				shotx = sp->x - screenx - 1;
				if (sp->lifecount == 0 && sp->strafedir >= 0)
					start_strafe(sp->x, sp->y, sp->strafedir);
				else if (shotx >= 0 && shotx < SCRWTH-3)
					draw_shipshot(shotx, shoty);
				else if (on_right_side &&
						 (shotx += worldwidth) >= 0 && shotx < SCRWTH-3)
					draw_shipshot(shotx, shoty);
			}
		}
}

move_bullets()
{
	register shotrec *sp, *end;
	register int left, right, top, bot;
	
	end = bunkshots+NUMSHOTS;
	left = globalx - SCENTER;
	right = globalx + SCENTER;
	top = globaly - SCENTER;
	bot = globaly + SCENTER;
	for (sp=bunkshots; sp < end; sp++)
		if (sp->lifecount)
		{	
			move_shot(sp);
			if (shielding && sp->x > left && sp->x < right &&
					sp->y > top && sp->y < bot && 
					xyindistance(sp->x - globalx,
								 sp->y - globaly, SHRADIUS))
			{
				sp->lifecount = sp->btime = 0;
				sp->strafedir = -1;
				continue;
			}
			if (sp->lifecount == 0 && sp->btime > 0)
			{
				backup_shot(sp);
				bounce_shot(sp);
			}
			DRAW_SHOT(sp);
		}
}

move_shot(sp)
register shotrec *sp;
{
	register int worldwth8, x, y;

	worldwth8 = worldwidth << 3;
	
	sp->lifecount--;
	x = sp->x8;
	y = sp->y8;
	x += sp->h;
	y += sp->v;
	if (y < 0)
		sp->lifecount = 0;
	if (x < 0)
		if (worldwrap)	x += worldwth8;
		else			sp->lifecount = 0;
	else if (x >= worldwth8)
		if (worldwrap)	x -= worldwth8;
		else			sp->lifecount = 0;
	sp->x8 = x;
	sp->y8 = y;
	x >>= 3;
	y >>= 3;
	sp->x = x;
	sp->y = y;
}


backup_shot(sp)		/* when shots bounce, back them up so they won't
						immediately hit the other side of the bounce wall */
register shotrec *sp;
{
	sp->x8 -= sp->h / 3;
	sp->y8 -= sp->v / 3;
	sp->x = sp->x8 >> 3;
	sp->y = sp->y8 >> 3;
}


/*  LEGAL_ANGLE
		Returns TRUE iff the bunker at rotation BASEROT and with the
	center of its base at BASEX, BASEY should be killed by a bullet
	coming from the direction of QUERYX, QUERYY.  Keeps bunkers from
	being shot from the back side of a wall.
*/

int angles360[16] =
{0, 22, 45, 67, 90, 112, 135, 157, 180, 202, 225, 247, 270, 292, 315, 337};

int legal_angle(baserot, basex, basey, queryx, queryy)
register int basex, basey;
int baserot, queryx, queryy;
{
	Rect therect;
	int angle;
	register int lowangle;
	Point pt;
	
	pt.v = queryy;				/* set Point for angle measurement */
	pt.h = queryx;

	therect.top	= basey - 20;	/* set rectangle for same */
	therect.bottom= basey + 20;
	therect.left	= basex - 20;
	therect.right	= basex + 20;
	
	PtToAngle(&therect, pt, &angle);	/* measure angle */
	
	lowangle = angles360[(baserot + 12) & 15];
	if (angle < lowangle)
		angle += 360;
	if (angle < lowangle + 180)
		return(TRUE);
	else
		return(FALSE);
}

bounce_shot(sp)
register shotrec *sp;
{
	/* theory:  v1 gets [v1 - 2(v1.v2)v2]
		v2 is unit vector that points outward from the bouncing wall.
	*/
	register int x1, y1, i;
	register long dot;
	
	for (i=0; i<8 && sp->lifecount==0 && sp->btime>0; i++)
	{
		x1 = bounce_vecs[sp->strafedir];
		y1 = bounce_vecs[(sp->strafedir+12)&15];
		dot = sp->h * x1 + sp->v * y1;
		sp->h -= x1 * dot / (24*48);
		sp->v -= y1 * dot / (24*48);
		sp->lifecount = sp->btime;
		sp->btime = 0;
		set_life(sp, sp->hitline);		/* find life before next bounce */
	}
	if (sp->lifecount == 0)
		sp->strafedir = -1;				/* no strafe if bounced out */
}



write_bonus()
{
	writeint(384, 12, planetbonus, back_screen);
	writeint(384, 12, planetbonus, front_screen);
}


fuel_minus(amount)
int amount;
{
	static char *fuelcritmess = "FUEL CRITICAL",
				*fuelgonemess = "OUT OF FUEL";
	
	if( (fuel -= amount) < 0)
		fuel = 0;
	if (fuel > MAXFUEL)
		fuel = MAXFUEL;
	
	fuelold = 2;			/* two screens to write new value on */
	
	if(fuel >= CRITFUEL && curmessage == fuelcritmess)
	{
		curmessage = NULL;
		new_sbar();
	}
	else if (fuel == 0)
	{	if(curmessage != fuelgonemess)
		{
			curmessage = fuelgonemess;
			new_sbar();
		}
	}
	else if (fuel < CRITFUEL && curmessage != fuelcritmess)
	{
		curmessage = fuelcritmess;
		new_sbar();
	}
}

int scpos;

score_plus(amount)
int amount;
{
	score += amount;
	scpos = score < 1000000 ? 216 : 224;
	writelong(scpos, 12, score, back_screen);
	writelong(scpos, 12, score, front_screen);
}


new_sbar()
{
	char *screen;
	register int pos, i;
	
	for (screen=back_screen; ; screen=front_screen)
	{
		sbar_clear(screen);
		
		for(i=0, pos=8; i < numships && pos < 150; i++, pos+=8)
			draw_digit(pos, 0, SHIPCHAR, screen);
		
		writestr(8, 12, curmessage, screen);		/* current message */
		writeint(456, 12, currentlevel, screen);
		
		if (screen == front_screen)
			break;
	}
	score_plus(0);	/* write score */
	fuel_minus(0);	/* write fuel */
	write_bonus();	/* write the bonus left */
}


update_sbar()
{
	if (fuelold)
	{
		fuelold--;
		writeint(296, 12, fuel, front_screen);
	}
}


writelong(x, y, n, screen)
int x, y;
long n;
char *screen;
{
	do
	{	draw_digit(x, y, (int) (n % 10), screen);
		n /= 10;
		x -= 8;
	} while (n > 0);
	draw_digit(x, y, SPACECHAR, screen);	/* clear extra digit */
}


writeint(x, y, n, screen)
register int x, y, n;
char *screen;
{
	asm
	{
			move.l	n, -(SP)
	@loop	and.l	#0x0000FFFF, n
			divs	#10, n
			swap	n
	}
	draw_digit(x, y, n, screen);
	asm
	{
			subq.w	#8, x
			swap	n
			tst.w	n
			bgt		@loop
	}
	draw_digit(x, y, SPACECHAR, screen);	/* clear extra digit */
	asm
	{
			move.l	(SP)+, n
	}
}


writestr(x, y, message, screen)
register int x, y;
register char *message, *screen;
{
	register char c;
	
	if ( !message)
		return;
	while(c = *message++)
	{
		if (c >= 'A' && c <= 'Z')
			draw_digit(x, y, c - ('A' - ACHAR), screen);
		x += 8;
	}
}



real_clear(screen)			/* clear the entire screen */
register char *screen;
{
	sbar_clear(screen);
	view_clear(screen);
}


init_screens()
{
	switch(Screen_Type)
	{
	case MACHINE_PLUS:	*change_screen |= 64;
						showing_primary = TRUE;
	case MACHINE_SE30:	front_screen = primary_screen;
						back_screen = secondary_screen;
						break;
	case MACHINE_SMALL:	
	case MACHINE_2:		front_screen = back_screen = secondary_screen;
									/* all drawing goes on "secondary" */
						break;
	}
	real_clear(front_screen);
}


swap_screens()		/* move fully drawn back screen to the front */
{
	switch(Screen_Type)
	{
	case MACHINE_2:
		CopyBits(&secondbmap, &game_window->portBits,
				&secondbmap.bounds, &game_window->portRect, srcCopy, NULL);
		break;
	case MACHINE_SMALL:
		OpenPort(&screen_port);
		CopyBits(&secondbmap, &screen_port.portBits,
				&secondbmap.bounds, &secondbmap.bounds, srcCopy, NULL);
		ClosePort(&screen_port);
		break;
	case MACHINE_SE30:
		copy_view(back_screen, front_screen);
		break;
	case MACHINE_PLUS:
		if(showing_primary)
		{	showing_primary = FALSE;
			front_screen = secondary_screen;
			back_screen  = primary_screen;
			*change_screen &= ~64;
		}
		else
		{	showing_primary = TRUE;
			front_screen = primary_screen;
			back_screen  = secondary_screen;
			*change_screen |= 64;
		}
		break;
	}
}


normal_screen()
{
	if (Screen_Type == MACHINE_PLUS)
	{
		*change_screen |= 64;
		thePort->portBits.baseAddr = primary_screen;
		showing_primary = TRUE;
	}
}



wait_for_VR()		/* synchronize with VR to avoid broken lines, and
						slow game down when there's nothing in view */
{
	static long now=0;
	
	tickwait = TickCount() - now;
	if (tickwait > 20 || tickwait < 0) tickwait = 9;
	while(TickCount() < now + 3)
		;
	now = TickCount();
}


/* XYINDIST:  Returns TRUE iff x^2 + y^2 < dist^2	*/
int xyindist(x, y, dist)
int x, y, dist;
{
	asm {
			move.w	dist(A6), D0
			cmp.w	x(A6), D0
			blt.s	@false
			cmp.w	y(A6), D0
			blt.s	@false
			neg.w	D0
			cmp.w	x(A6), D0
			bgt.s	@false
			cmp.w	y(A6), D0
			bgt.s	@false
			
			move.w	x(A6), D1
			move.w	y(A6), D2
			muls	D0, D0
			muls	D1, D1
			muls	D2, D2
			add.w	D2, D1
			cmp.w	D0, D1
			bgt.s	@false
			move.w	#1, D0
			bra.s	@out
	
	@false	move.w	#0, D0
	@out	
	}
}


/* XYINDISTANCE:  like XYINDIST, but assumes x,y in 2dist X 2dist rect */
int xyindistance(x, y, dist)
int x, y, dist;
{
	asm {
			move.w	dist(A6), D2
			move.w	x(A6), D1
			move.w	y(A6), D0
			muls	D0, D0
			muls	D1, D1
			muls	D2, D2
			add.w	D0, D1
			moveq	#FALSE, D0
			cmp.w	D2, D1
			bgt.s	@false
			moveq	#TRUE, D0
	@false	
	}
}


#define	STARTGAP	40
#define	ENDGAP		5

crackle()
{
	register int gap, i;
	long dummylong;
	
	if (Screen_Type == MACHINE_2 ||
		Screen_Type == MACHINE_SMALL)	/* different front, back screen-widths */
		return;
	star_background();
	start_sound(FIZZ_SOUND);
	fizz(back_screen, front_screen);
	start_sound(ECHO_SOUND);
	Delay(150L, &dummylong);
}


star_background()
{
	register int i;
	
	set_screen(back_screen, -1L);
	for (i=0; i<150; i++)
		clear_point(rint(SCRWTH), rint(VIEWHT));
	if (!dead_count)
		full_figure(shipx-SCENTER, shipy-SCENTER,
					ship_defs[shiprot], ship_masks[shiprot], SHIPHT);
}



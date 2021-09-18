/* 
	File Titlepage.c:  code for title page stuff in Magnetron
	
	Copyright � 1987 Randall H. Wilson
*/

#include <Quickdraw.h>
#include <WindowMgr.h>
#include <MemoryMgr.h>
#include <asm.h>
#include "GW.h"

#define	STARHEIGHT	160
#define	STARWIDTH	512

#define	NUMSTARS	100

RgnHandle windshield_rgn();

BitMap starbmap;
GrafPort starport;
WindowPtr win;


main()
{
	WindowRecord winrec;
	Rect boundsrect;
	RgnHandle view;
	static int bigrect[4] = {0, 0, 1000, 1000};
	
	init();
	
	SETRECT(boundsrect, 0, 20, 512, 342);
	win = NewWindow(&winrec, &boundsrect, "\p", TRUE, 1, -1L, FALSE, 0L);
	
	init_stars();
	
	SetPort(win);
	PaintRect(&bigrect);
	view = windshield_rgn();
	SetClip(view);
	
	while (!Button())
		update_stars();
		
	ClosePort(&starport);
	CloseWindow(win);
}
	
	
	
init()
{
	InitGraf(&thePort);
	InitFonts();
	InitWindows();
	InitCursor();
	TEInit();
	InitDialogs(NULL);
	InitMenus();
}



RgnHandle windshield_rgn()
{
	register RgnHandle thergn;
	
	thergn = NewRgn();
	
	OpenRgn();
	
		MoveTo(20, 0);
		LineTo(480, 0);
		LineTo(300, 120);
		LineTo(200, 120);
		LineTo(20, 0);
		
		MoveTo(0, 10);
		LineTo(180, 130);
		LineTo(0, 160);
		LineTo(0, 10);
		
		MoveTo(500, 10);
		LineTo(320, 130);
		LineTo(500, 160);
		LineTo(500, 10);
		
	CloseRgn(thergn);
	
	return(thergn);
}


#define	BIGSTAR	10

int star0[2] = {0, 0x7FFF},
	star1[2] = {0, 0x3FFF},
	star2[3] = {1, 0x3FFF, 0x3FFF},
	star3[4] = {2, 0xBFFF, 0x1FFF, 0xBFFF},
	star4[4] = {2, 0x9FFF, 0x0FFF, 0x9FFF},
	star5[5] = {3, 0x9FFF, 0x0FFF, 0x0FFF, 0x9FFF},
	star6[6] = {4, 0x8FFF, 0x07FF, 0x07FF, 0x07FF, 0x8FFF},
	star7[7] = {5, 0xCFFF, 0x87FF, 0x03FF, 0x03FF, 0x87FF, 0xCFFF},
	star8[8] = {6, 0xC7FF, 0x83FF, 0x01FF, 0x01FF, 0x01FF, 0x83FF, 0xC7FF},
	star9[9] = {7, 0xC3FF, 0x81FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
						0x81FF, 0xC3FF},
	star10[10]={8, 0xC1FF, 0x80FF, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F,
						0x80FF, 0xC1FF};
	
int *starpicts[BIGSTAR+1] = {star0, star1, star2, star3, star4, star5,
							star6, star7, star8, star9, star10};

typedef struct
{
	int x, xslow, y, yslow, speed, size;
} starrec;

starrec stars[NUMSTARS];


init_stars()
{
	register starrec *star;
	
	OpenPort(&starport);
	SetPort(&starport);
	starbmap.baseAddr = NewPtr((long) (STARHEIGHT+12) * 64);
	starbmap.rowBytes = 64;
	SETRECT(starbmap.bounds, 0, 0, 512, STARHEIGHT);
	SetPortBits(&starbmap);
	
	for (star=stars; star < stars + NUMSTARS; star++)
	{
		new_star(star);
		star->speed = rint(60);
	}
}


#define	CENTERX	250
#define	CENTERY	70


update_stars()
{
	register starrec *star, *end;
	register long dx, dy, width, height;
	int size;
	Point mousepos;
	Rect starrect;
	
	SetPort(&starport);
	
	GetMouse(&mousepos);
	dx = (512/2 - mousepos.h) * 1024L;
	dy = (mousepos.v - 342/2) * 1300L;
	width = (long) STARWIDTH << 16;
	height = (long) STARHEIGHT << 16;
	
	clear_starbmap();
	
	end = stars + NUMSTARS;
	for (star=stars; star < end; star++)
	{
		asm
		{
				move.l	OFFSET(starrec, x)(star), D1
				add.l	dx, D1
				bge.s	@1
				add.l	width, D1
				bra.s	@storex
		@1		cmp.l	width, D1
				blt.s	@storex
				sub.l	width, D1
		@storex	move.l	D1, OFFSET(starrec, x)(star)
				
				move.l	OFFSET(starrec, y)(star), D1
				add.l	dy, D1
				bge.s	@2
				add.l	height, D1
				bra.s	@storey
		@2		cmp.l	height, D1
				blt.s	@storey
				sub.l	height, D1
		@storey	move.l	D1, OFFSET(starrec, y)(star)
		}
				
		star->speed += (star->speed >> 5) + 1;
/*		*((long *) &star->x) += star->speed * (long) (star->x - CENTERX);
		*((long *) &star->y) += star->speed * (long) (star->y - CENTERY);
*/		asm
		{								/* code to do above 2 lines fast */
				move.w	OFFSET(starrec, x)(star), D0
				sub.w	#CENTERX, D0
				muls	OFFSET(starrec, speed)(star), D0
				add.l	D0, OFFSET(starrec, x)(star)
				move.w	OFFSET(starrec, y)(star), D0
				sub.w	#CENTERY, D0
				muls	OFFSET(starrec, speed)(star), D0
				add.l	D0, OFFSET(starrec, y)(star)
		}
		if (star->x < 0 || star->x >= STARWIDTH ||
			star->y < 0 || star->y >= STARHEIGHT || star->speed > 29000)
				new_star(star);
		
		if ((size = star->speed >> 8) > BIGSTAR)
		{
			SETRECT(starrect, star->x, star->y,
						star->x + size, star->y + size);
			FillOval(&starrect, white);
		}
		else
			asm
			{
					move.l	starbmap.baseAddr(A5), A0
					move.w	OFFSET(starrec, y)(star), D0
					asl.w	#6, D0
					adda.w	D0, A0
					move.w	OFFSET(starrec, x)(star), D1
					move.w	D1, D0
					asr.w	#4, D0
					asl.w	#1, D0
					adda.w	D0, A0
					and.w	#15, D1
					move.w	size(A6), D2
					bne.s	@drawit
					move.w	#0x7FFF, D0
					ror.w	D1, D0
					and.w	D0, (A0)
					bra.s	@leave
			
			@drawit	asl.w	#2, D2
					lea		starpicts(A5), A1
					move.l	0(A1, D2.w), A1
					
					move.w	(A1)+, D2
			@drawlp	moveq	#-1, D0
					move.w	(A1)+, D0
					swap	D0
					ror.l	D1, D0
					and.l	D0, (A0)
					adda.w	#64, A0
					dbra	D2, @drawlp
			@leave
			}
	}
	SetPort(win);
	CopyBits(&starport.portBits, &win->portBits,
			&starport.portRect, &starport.portRect,
			srcCopy, NULL);
}


clear_starbmap()
{
	asm
	{
			movem.l	D3-D7, -(SP)
			move.l	starbmap.baseAddr(A5), A0
			adda.w	#STARHEIGHT*64, A0			/* move to end, go back */
			moveq	#-1, D1
			move.l	D1, D2
			move.l	D2, D3
			move.l	D3, D4
			move.l	D4, D5
			move.l	D5, D6
			move.l	D6, D7
			move.l	D7, A1
			
			move.w	#STARHEIGHT-1, D0
			
	@loop	movem.l	D1-D7/A1, -(A0)
			movem.l	D1-D7/A1, -(A0)
			dbra	D0, @loop
			
			movem.l	(SP)+, D3-D7
	}
}



new_star(star)
register starrec *star;
{
	star->x = rint(STARWIDTH);
	star->y = rint(STARHEIGHT);
	star->speed = 0;
}


int rint(n)			/* returns an integer from 0 to n-1	*/
int n;
{
	asm
	{		subq.w	#2, SP
			_Random			/* trap to random */
			move.w	(SP)+, D0
			mulu	n(A6), D0
			swap	D0
	}					/* might want to speed this up by table of rands */
}




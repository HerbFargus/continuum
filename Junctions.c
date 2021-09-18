/*
	Junctions.c:  init code for Walls, and other intelligent stuff for
		junctions, etc.
	
	Copyright (c) 1986-88 Randall H. Wilson
*/ 


#include "GW.h"
#include "Assembly Macros.h"



extern linerec lines[];
extern linerec *kindptrs[], *firstwhite;
extern long background[2], backgr1, backgr2;
extern char *back_screen;
extern int screenx, screeny, screenb, screenr, worldwidth;

typedef struct {int x, y;} junctionrec;

junctionrec junctions[NUMLINES*2+20];
int numjunctions;

#define	LEFT_CLIP	0x0000FFFFL
#define	RIGHT_CLIP	0xFFFF0000L
#define	CENTER_CLIP	0xFFFFFFFFL


/*
	Init_Walls():  does all the preliminary work to be ready for
		drawing walls on the current level.
*/
init_walls()
{
	register linerec *line;
	linerec **last;
	register junctionrec *j;
	junctionrec *movej, *lastj, tempjcn;
	register int kind, x, y;
	int i;
	
	for (kind = L_NORMAL; kind < L_NUMKINDS; kind++)
	{
		last = &kindptrs[kind];
		for (line=lines; line->type; line++)
			if (line->kind == kind)
			{
				*last = line;
				last = &line->next;
			}
		*last = NULL;
	}
	last = &firstwhite;
	for (line=lines; line->type; line++)
		if (line->newtype == NEW_NNE)
		{
			*last = line;
			last = &line->nextwh;
		}
	*last = NULL;
	
	junctions[0].x = 20000;
	numjunctions = 0;
	lastj = junctions;
	for (line=lines; line->type; line++)
		for (i=0; i<2; i++)
		{
			x = (i ? line->endx : line->startx);
			y = (i ? line->endy : line->starty);
			for (j=junctions; j < lastj; j++)
				if (j->x <= x+3 && j->x >= x-3 &&
				    j->y <= y+3 && j->y >= y-3)
					break;
			if (j == lastj)
			{
				lastj->x = x;
				lastj->y = y;
				(++lastj)->x = 20000;
				numjunctions++;
			}
		}
			/* sort the junctions by x */
	for (j=junctions; j<lastj; j++)
		for (movej = j; movej > junctions && 
			movej->x < (movej-1)->x; movej--)
		{
			tempjcn = *movej;
			*movej = *(movej-1);
			*(movej-1) = tempjcn;
		}
	for (i=0; i<18; i++)
		junctions[numjunctions+i].x = 20000;
	
	init_whites();
}

int hash_figure[6] = {0x8000, 0x6000, 0x1800, 0x0600, 0x00180, 0x0040};

int neglitch[4] = {0xEFFF, 0xCFFF, 0x8FFF, 0x0FFF};
int eneglitch1[3] = {0x07FF, 0x1FFF, 0x7FFF};
int eneglitch2[5] = {0xFF3F, 0xFC3F, 0xF03F, 0xC03F, 0x003F};
int eseglitch[4] = {0x3FFF, 0xCFFF, 0xF3FF, 0xFDFF};

int generictop[6] = {0xFFFF, 0x3FFF, 0x0FFF, 0x03FF, 0x00FF, 0x007F};
int nnebot[6] =	{0x800F, 0xC01F, 0xF01F, 0xFC3F, 0xFF3F, 0xFFFF};
int nebot[6] =	{0x8001, 0xC003, 0xF007, 0xFC0F, 0xFF1F, 0xFFFF};
int eneleft[6]=	{0x8000, 0xC000, 0xF000, 0xFC01, 0xFF07, 0xFFDF};
int eleft[6] =	{0xFFFF, 0xFFFF, 0xF000, 0xFC00, 0xFF00, 0xFF80};
int eseright[6]={0xFFFF, 0x3FFF, 0x8FFF, 0xE3FF, 0xF8FF, 0xFE7F};
int setop[6] =	{0xFFFF, 0xFFFF, 0xEFFF, 0xF3FF, 0xF8FF, 0xFC3F};
int sebot[6] =	{0x87FF, 0xC3FF, 0xF1FF, 0xFCFF, 0xFF7F, 0xFFFF};
int ssetop[6] =	{0xFFFF, 0xBFFF, 0xCFFF, 0xC3FF, 0xE0FF, 0xE03F};
int ssebot[6] =	{0x80FF, 0xC07F, 0xF07F, 0xFC3F, 0xFF3F, 0xFFFF};
int sbot[6] =	{0x803F, 0xC03F, 0xF03F, 0xFC3F, 0xFF3F, 0xFFFF};


int *whitepicts[][2] =
{	{NULL, NULL},
	{generictop, sbot},
	{ssetop, ssebot},
	{setop, sebot},
	{NULL, eseright},
	{eleft, generictop},
	{eneleft, generictop},
	{nebot, generictop},
	{nnebot, generictop}};
	
typedef int whitedata;

typedef struct
{
	int x, y, hasj, ht;
	whitedata *data;
} whiterec;

int *whitestorage = NULL;	/* [NUMLINES*2+20][6] */
int whitesused;

whiterec *whites = NULL;	/* [NUMLINES*4+20] */
int numwhites;


add_white(x, y, ht, data)
int x, y, ht;
int *data;
{
	register whiterec *wh = whites + numwhites;
	
	wh->x = x;
	wh->y = y;
	wh->ht = ht;
	wh->data = data;
	wh->hasj = FALSE;
	numwhites++;
	wh++;
	wh->x = 20000;
}



replace_white(x, y, ht, data)
int x, y, ht;
int *data;
{
	replace_white_2(x, y, x, y, ht, data);
}



replace_white_2(targetx, targety, x, y, ht, data)
int targetx, targety, x, y, ht;
int *data;
{
	register whiterec *wh;
	
	for (wh=whites; wh < whites + numwhites &&
		(wh->y != targety || wh->x != targetx || wh->ht >= ht); wh++)
		;
	if (wh >= whites + numwhites)
		return;
	wh->x = x;
	wh->y = y;
	wh->ht = ht;
	wh->data = data;
}

/*	for ease of calling add_white along with replace_white_2
*/
add_white_2(dummyx, dummyy, x, y, ht, data)
int dummyx, dummyy, x, y, ht;
int *data;
{
	add_white(x, y, ht, data);
}



init_whites()
{
	whiterec *wh, *movewh, tempwh;
	register int i;
	int *newdata;
		
	whitesused = 0;
	numwhites = 0;
	
	norm_whites();
	
	close_whites();
	
			/* insertion sort the whites */
	for (wh=whites; wh < whites + numwhites; wh++)
		for (movewh = wh; movewh > whites &&
			movewh->x <= (movewh-1)->x &&
			(movewh->x < (movewh-1)->x ||
			 movewh->y < (movewh-1)->y); movewh--)
		{
			tempwh = *movewh;
			*movewh = *(movewh-1);
			*(movewh-1) = tempwh;
		}
	
	for (i=0; i<18; i++)
		whites[numwhites+i].x = 20000;

		/* merge whites together */
	for (wh=whites; wh->x < 20000; wh++)
		while (wh->x == (wh+1)->x && wh->y == (wh+1)->y &&
			wh->ht == 6 && (wh+1)->ht == 6)
		{
			newdata = whitestorage + whitesused;
			whitesused += 6;
			for (i=0; i<6; i++)
				newdata[i] = wh->data[i] & (wh+1)->data[i];
			wh->data = newdata;
			for (movewh = wh+1; movewh->x < 20000; movewh++)
				*movewh = movewh[1];
			numwhites--;
		}
	white_hash_merge();
}


norm_whites()
{
	register linerec *line;
	register int i;
	
	for (line=lines; line->type; line++)
	{
		for (i=0; i<2; i++)
			if (whitepicts[line->newtype][i])
				add_white(
					(i ? line->endx : line->startx),
					(i ? line->endy : line->starty),
					6,
					whitepicts[line->newtype][i]);
		switch(line->newtype)
		{
		case NEW_NE:
			add_white(line->endx - 4, line->endy + 2,
					4, neglitch);
			break;
		case NEW_ENE:
			add_white(line->startx + 16, line->starty,
				3, eneglitch1);
			add_white(line->endx - 10, line->endy + 1,
				5, eneglitch2);
			break;
		case NEW_ESE:
			add_white(line->endx - 7, line->endy - 2,
				4, eseglitch);
			break;
		default:
			break;
		}
	}
}

int simpleh1[9] = {0, 6, 6, 6, 12, 16, 0, 1, 0};
int simpleh2[9] = {0, 0, 0, 0, -1, 0,-11,-5,-5};

int npatch[22];

close_whites()
{
	register linerec *line, *line2;
	linerec *first;
	register int x1, y1, x2, y2;
	int i, j;
	
	for (j=0; j<22; j++)
		npatch[j] = 0x003F;
	
	for (line=lines; line->type; line++)
	{
		line->h1 = simpleh1[line->newtype];
		line->h2 = line->length + simpleh2[line->newtype];
	}
	
	first = lines;
	for (line=lines; line->type; line++)
	{
	    while (first->endx < line->startx - 3)
	    	first++;
	    for (i=0; i<2; i++)
	    {
	    	x1 = (i ? line->endx : line->startx);
	    	y1 = (i ? line->endy : line->starty);
	    	for (line2=first; line2->startx < x1 + 3; line2++)
		    for (j=0; j<2; j++)
		    {
		    	x2 = (j ? line2->endx : line2->startx) - 3;
		    	y2 = (j ? line2->endy : line2->starty) - 3;
		    	if (x1 > x2 && y1 > y2 &&
		    			x1 < x2 + 6 && y1 < y2 + 6)
		    		one_close(line, line2, i, j);
		    }
	    }
	}
}


int nepatch[4] = {0xE000, 0xC001, 0x8003, 0x0007};
int enepatch[4] = {0xFC00, 0xF003, 0xC00F, 0x003F};
int epatch[4] = {0x0003, 0x0003, 0x0003, 0x0003};
int sepatch[11] = {0x07FF, 0x83FF, 0xC1FF, 0xE0FF, 0xF07F, 0xF83F, 0xFC1F,
			0xFE0F, 0xFF07, 0xFF83, 0xFFC1};
int ssepatch[18] = {0x00FF, 0x00FF, 0x807F, 0x807F, 0xC03F, 0xC03F,
			0xE01F, 0xE01F, 0xF00F, 0xF00F, 0xF807, 0xF807,
			0xFC03, 0xFC03, 0xFE01, 0xFE01, 0xFF00, 0xFF00};

one_close(line, line2, n, m)
register linerec *line, *line2;
int n, m;
{
	register int dir1, dir2, i, j;
	
	dir1 = 9 - line->newtype;
	if (n)
		dir1 = (dir1 + 8) & 15;
	dir2 = 9 - line2->newtype;
	if (m)
		dir2 = (dir2 + 8) & 15;
	
	if (dir1 == dir2)
		return;
	switch(dir1)
	{
	case 0:
		switch (dir2)
		{
		case 15:
		case 1:
			i = 21;
			break;
		case 2:
			i = 10;
			break;
		case 3:
		case 14:
			i = 6;
			break;
		default:
			return;
		}
		j = line->h2;
		if (line->length - i > j)
			return;
		(*(j < line->length ? replace_white_2 : add_white_2))
			(line->startx, line->starty + j,
			 line->endx, line->endy - i, i, npatch);
		line->h2 = line->length - i;
		break;
	case 1:
		break;
	case 2:
		switch (dir2)
		{
		case 0:
			i = 3;
			break;
		case 1:
			i = 6;
			break;
		case 3:
			i = 4;
			break;
		case 14:
			i = 1;
			break;
		case 15:
			i = 2;
			break;
		default:
			return;
		}
		for (j = 0; j < 4*i; j += 4)
			if (line->h1 < 5 + j)
				add_white(line->startx + 3 + j,
					line->starty - 4 - j,
					4, nepatch);
		i--;
		j = 5 + 4*i;
		if (line->h1 < j)
			line->h1 = j;
	case 3:
	case 4:
	case 5:
		break;
	case 6:
		if (dir2 == 7)
			i = 11;
		else if (dir2 > 7 && dir2 < 12)
			i = 5;
		else
			return;
		if (line->h1 >= 6+i)
			return;
		(*(line->h1 > 6 ? replace_white : add_white))
			(line->startx + 6, line->starty + 6, i, sepatch);
		line->h1 = 6 + i;
		break;
	case 7:
		if (dir2 == 6 || dir2 == 8)
			i = 16;
		else if (dir2 == 9)
			i = 8;
		else if (dir2 > 9 && dir2 < 12)
			i = 6;
		else
			return;
		if (line->h1 >= 6 + i)
			return;
		(*(line->h1 > 6 ? replace_white : add_white))
			(line->startx + 3, line->starty + 6, i, ssepatch);
		line->h1 = 6 + i;
		break;
	case 8:
		switch (dir2)
		{
		case 6:
		case 11:
			i = 5;
			break;
		case 10:
			i = 10;
			break;
		case 9:
		case 7:
			i = 20;
			break;
		default:
			return;
		}
		if (i+6 < line->h1)
			return;
		(*(line->h1 > 6 ? replace_white : add_white))
			(line->startx, line->starty + 6, i, npatch);
		line->h1 = i + 6;
		break;
	case 9:
		break;
	case 10:
		switch(dir2)
		{
		case 6:
		case 7:
		case 8:
			i = dir2 - 5;
			break;
		case 9:
			i = 6;
			break;
		case 11:
			i = 4;
			break;
		default:
			return;
		}
		for (j=0; j < 4*i; j += 4)
			if (line->h2 > line->length - 9 - j)
				add_white(line->endx - 7 - j,
					line->endy + 6 + j,
					4, nepatch);
		i--;
		j = line->length - 9 - 4*i;
		if (line->h2 > j)
			line->h2 = j;
	case 11:
		if (dir2 == 9)
			i = 2;
		else if (dir2 == 10)
			i = 4;
		else
			return;
		for (j=0; j < 8*i; j += 8)
			if (line->h2 >= line->length - 11 - j)
			{
				add_white(line->endx - 18 - j,
					line->endy + 6 + (j>>1),
					4, enepatch);
				add_white(line->endx - 8 - j,
					line->endy + 6 + (j>>1),
					4, enepatch);
			}
		j = line->length - 11 - 8*i;
		if (line->h2 > j)
			line->h2 = j;
		break;
	case 12:
		if (dir2 > 8 && dir2 < 12 && line->h2 == line->length)
		{
			add_white(line->endx - 14, line->endy+2,
				4, epatch);
			line->h2 = line->length - 14;
		}
		break;
	case 13:
		break;
	case 14:
		if (dir2 == 15)
			i = 10;
		else if (dir2 < 4 || dir2 == 13)
			i = 5;
		else
			return;
		j = line->h2;
		if (j <= line->length - i)
			return;
		(*(j < line->length ? replace_white_2 : add_white_2))
			(line->startx + j, line->starty + j,
			 line->endx - i, line->endy - i, i, sepatch);
		line->h2 = line->length - i;
		break;
	case 15:
		switch (dir2)
		{
		case 0:
			i = 17;
			break;
		case 1:
		case 14:
			i = 11;
			break;
		case 2:
		case 3:
			i = 5;
			break;
		default:
			return;
		}
		j = line->h2;
		if (j <= line->length - i)
			return;
		(*(j < line->length ? replace_white_2 : add_white_2))
			(line->startx + (j >> 1), line->starty + j,
			 line->endx - (i>>1), line->endy - i, i, ssepatch);
		line->h2 = line->length - i;
		break;
	default:
		break;
	}
}



white_hash_merge()
{
	register whiterec *wh;
	register junctionrec *j;
	junctionrec *movej;
	register int i, back, x;
	int *newdata;
	
	j = junctions;
	for (wh=whites; wh->x < worldwidth - 8; wh++)
	    if (wh->ht == 6 && no_close_wh(wh) && wh->x > 8)
	    {
		while (j > junctions && j->x >= wh->x)
			j--;
		while (j->x <= wh->x && j->y != wh->y)
			j++;
		if (j->x == wh->x && j->y == wh->y)
		{
			back = (((wh->x + wh->y) & 1) ? 
						backgr2 : backgr1);
			if (wh->data >= whitestorage &&
				wh->data < whitestorage + whitesused)
				newdata = wh->data;
			else
			{
				newdata = whitestorage + whitesused;
				whitesused += 6;
			}
			for (i=0; i<6; i++)
			{
				newdata[i] =
					(back &
					 (~wh->data[i] | hash_figure[i]))
					^ hash_figure[i];
				asm {
					rol.w	#1, back
				}
			}
			wh->data = newdata;
			wh->hasj = TRUE;
			for (movej = j; movej->x < 20000; movej++)
				*movej = movej[1];
			numjunctions--;
		}
	    }
}


int no_close_wh(w1)
register whiterec *w1;
{
	register whiterec *w2;
	
	for (w2=w1-1; w2 >= whites && w2->x > w1->x - 3; w2--)
		if (w2->y < w1->y + 3 && w2->y > w1->y - 3)
			return(FALSE);
	for (w2=w1+1; w2->x < w1->x + 3; w2++)
		if (w2->y < w1->y + 3 && w2->y > w1->y - 3)
			return(FALSE);
	return(TRUE);
}
	



fast_whites()
{
	register whiterec *wh;
	register int top, left, right, bot;
	int i;
	int white_wall_piece(), eor_wall_piece();
	
	top = screeny;
	left = screenx - 15;
	bot = screenb;
	right = screenr;
	for (i=0; i<2; i++)
	{
		asm
		{
			move.l	D3, -(SP)
			
			move.l	whites(A5), wh
			move.w	#16*sizeof(whiterec), D3
			
		@fast	adda.w	D3, wh
			cmp.w	OFFSET(whiterec, x)(wh), left
			bgt.s	@fast
			suba.w	D3, wh
			moveq	#sizeof(whiterec), D3
			bra.s	@enterf
			
		@find	adda.w	D3, wh
		@enterf	cmp.w	OFFSET(whiterec, x)(wh), left
			bgt.s	@find
			
			add.w	#15, left
			bra.s	@enter
		
		@loop	adda.w	D3, wh
		@enter	move.w	OFFSET(whiterec, x)(wh), D0
			cmp.w	D0, right
			ble.s	@leave
			
			move.w	OFFSET(whiterec, y)(wh), D1
			cmp.w	D1, bot
			blt.s	@loop
			sub.w	top, D1
			move.w	OFFSET(whiterec, ht)(wh), D2
			neg.w	D2
			cmp.w	D2, D1
			ble.s	@loop
			
			neg.w	D2
			sub.w	left, D0
			
			move.w	D2, -(SP)
			move.l	OFFSET(whiterec, data)(wh), -(SP)
			move.w	D1, -(SP)
			move.w	D0, -(SP)
			tst.w	OFFSET(whiterec, hasj)(wh)
			beq.s	@white
			jsr	eor_wall_piece
			adda.w	#10, SP
			bra.s	@loop
			
		@white	jsr	white_wall_piece
			adda.w	#10, SP
			bra.s	@loop
						
		@leave	sub.w	#15, left
			move.l	(SP)+, D3
		}
				
		left -= worldwidth;
		right -= worldwidth;
	}
}


/* take args in register variables for speed? */

white_wall_piece(x, y, def, height)
register int x, y, height;
register char *def;			/* really (int *) */
{
	register long clip;
	
	if (y < 0)
	{
		if ((height += y) <= 0)
			return;
		def -= y << 1;
		y = 0;
	}
	else if (y + height > VIEWHT)
	{
		if (y >= VIEWHT)
			return;
		height = VIEWHT - y;
	}
	clip = ~CENTER_CLIP;
	if (x < 0)
	{
		if (x <= -16)
			return;
		clip = ~LEFT_CLIP;
	}
	else if (x >= SCRWTH-16)
	{
		if (x >= SCRWTH)
			return;
		clip = ~RIGHT_CLIP;
	}
	
	y += SBARHT;
	asm
	{
		FIND_WADDRESS(x,y)
		
		and.w	#15, x
		neg.w	x
		add.w	#16, x
		moveq	#64, D1
		bra.s	@enter
		
	@loop	moveq	#-1, D0
		move.w	(def)+, D0
		rol.l	x, D0
		or.l	clip, D0
		and.l	D0, (A0)
		adda.l	D1, A0
	@enter	dbra	height, @loop
	}
}


eor_wall_piece(x, y, def, height)
register int x, y, height;
register char *def;			/* really (int *) */
{
	register long clip;
	
	if (y < 0)
	{
		if ((height += y) <= 0)
			return;
		def -= y << 1;
		y = 0;
	}
	else if (y + height > VIEWHT)
	{
		if (y >= VIEWHT)
			return;
		height = VIEWHT - y;
	}
	clip = CENTER_CLIP;
	if (x < 0)
	{
		if (x <= -16)
			return;
		clip = LEFT_CLIP;
	}
	else if (x >= SCRWTH-16)
	{
		if (x >= SCRWTH)
			return;
		clip = RIGHT_CLIP;
	}
	
	y += SBARHT;
	asm
	{
		FIND_WADDRESS(x,y)
		
		and.w	#15, x
		neg.w	x
		add.w	#16, x
		moveq	#64, D1
		bra.s	@enter
		
	@loop	moveq	#0, D0
		move.w	(def)+, D0
		rol.l	x, D0
		and.l	clip, D0
		eor.l	D0, (A0)
		adda.l	D1, A0
	@enter	dbra	height, @loop
	}
}



fast_hashes()
{
	register junctionrec *j;
	register int top, left, right, bot;
	int i;
	void draw_hash();
	
	top = screeny - 5;
	left = screenx - 8;
	bot = screenb;
	right = screenr;
	for (i=0; i<2; i++)
	{
	asm
	{
		lea	junctions(A5), j
		move.w	#sizeof(junctionrec)*16, D0
		
		/* find first one to draw */
	@find16	adda.w	D0, j
		cmp.w	OFFSET(junctionrec, x)(j), left
		bgt.s	@find16
		suba.w	D0, j
		bra.s	@enter1
			
	@loop1	addq.l	#sizeof(junctionrec), j
	@enter1	cmp.w	OFFSET(junctionrec, x)(j), left
		bgt.s	@loop1
		
		addq.w	#8, left
		bra	@enter2
		
	@loop2	move.w	OFFSET(junctionrec, y)(j), D2
		cmp.w	top, D2
		blt.s	@skip
		cmp.w	bot, D2
		bge.s	@skip
		
		move.w	OFFSET(junctionrec, x)(j), D1
		sub.w	left, D1
		sub.w	screeny(A5), D2
		blt.s	@jsr_it
		cmp.w	#VIEWHT-5, D2
		bge.s	@jsr_it
		tst.w	D1
		blt.s	@jsr_it
		cmp.w	#SCRWTH-9, D1
		blt.s	@do_quick
		
	@jsr_it	move.w	D2, -(SP)
		move.w	D1, -(SP)
		jsr	draw_hash
		addq.w	#4, SP
		
	@skip	addq.l	#sizeof(junctionrec), j
	@enter2	cmp.w	OFFSET(junctionrec, x)(j), right
		bgt.s	@loop2
		bra	@leave
		
	@do_quick	
		add.w	#SBARHT, D2
		FIND_WADDRESS(D1, D2)
		and.w	#15, D1
		move.l	#0x80000000, D0
		lsr.l	D1, D0
		
		or.l	D0, (A0)
		move.l	D0, D1
		lsr.l	#1, D1
		or.l	D1, D0
		lsr.l	#1, D0
		or.l	D0, 64*1(A0)
		lsr.l	#2, D0
		or.l	D0, 64*2(A0)
		lsr.l	#2, D0
		or.l	D0, 64*3(A0)
		lsr.l	#2, D0
		or.l	D0, 64*4(A0)
		move.l	D0, D1
		lsr.l	#2, D1
		lsr.l	#1, D0
		and.l	D1, D0
		or.l	D0, 64*5(A0)
		bra	@skip
		
	@leave	subq.w	#8, left
	}
		right -= worldwidth;
		left -= worldwidth;
	}
}



void draw_hash(x, y)
register int x, y;
{
	register char *data = (char *) hash_figure;
	register long clip;
	register int height;
	
	height = 6;
	if (y < 0)
	{
		height += y;
		data -= y << 1;
		y = 0;
	}
	else if (y >= VIEWHT-6)
	{
		height = VIEWHT - y;
	}
	if (x < 0)
		clip = LEFT_CLIP;
	else if (x >= SCRWTH - 9)
		clip = RIGHT_CLIP;
	else
		clip = CENTER_CLIP;
	
	y += SBARHT;
	
	asm
	{
		FIND_WADDRESS(x,y)
		
		and.w	#15, x
		neg.w	x
		add.w	#16, x
		moveq	#64, D1
		subq.w	#1, height
		blt.s	@leave
		
	@loop	moveq	#0, D0
		move.w	(data)+, D0
		rol.l	x, D0
		and.l	clip, D0
		or.l	D0, (A0)
		adda.l	D1, A0
		dbra	height, @loop
	@leave
	}	
}
		
		

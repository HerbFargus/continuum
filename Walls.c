/* 
	Walls.c:  3-dimensional walls for Gravity Well
	
	May Krsna help me.
	
	Copyright (c) 1986-88 Randall H. Wilson
*/

#include	"GW.h"
#include	"Assembly Macros.h"


extern long background[2];
extern char *back_screen;
extern int screenx, screeny, screenb, screenr;

#define	LEFT_CLIP		0x0000FFFFL
#define	RIGHT_CLIP	0xFFFF0000L
#define	CENTER_CLIP	0xFFFFFFFFL


#define	NNE_MASK	0x000FFFFF

/*
	NNE_Black:  draws black parts of a NNE line.
*/
void nne_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len;
	long eor1, eor2;
	int start, end;
	int h1, h2, h3, h4;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h4 = line->length + 1;
	
	if (h4 & 1)
		h4++;
	
	if (x + (h1>>1) < 0)
		h1 = -x << 1;
	if (y - h1 > VIEWHT-1)
		h1 = y - (VIEWHT-1);
	if (h1 & 1)
		h1++;
	if (x + (h4>>1) > SCRWTH)
		h4 = (SCRWTH - x) << 1;
	if (y - h4 < -1)
		h4 = y + 1;
	
	if (h4 > h1)
	      draw_nneline(x+(h1>>1), y - h1 + SBARHT, h4 - h1 - 1, L_UP);
}


/*
	NNE_White:  draws white parts of a NNE line.
*/
void nne_white(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len;
	int h1, h2, h3, h4, leftx, lefty, start, end;
	
	x = line->startx - scrx;
	y = line->starty - scry;
		
	h1 = 0;
	h4 = line->length - 5;
	
	if (x + (h1>>1) < -11)
		h1 = (-11-x) << 1;
	if (y - h1 > VIEWHT-1)
		h1 = y - (VIEWHT-1);
	if (h1 & 1)
		h1++;
	if (x + (h4>>1) > SCRWTH)
		h4 = (SCRWTH - x) << 1;
	if (y - h4 < -1)
		h4 = y + 1;
	if (h4 & 1)
		h4--;
	h2 = h1;
	if (x + (h2>>1) < 0)
		h2 = -x << 1;
	if (h2 & 1)
		h2++;
	if (h2 > h4)
		h2 = h4;
	h3 = h4;
	if (x + (h3>>1) > SCRWTH-12)
		h3 = (SCRWTH-12 - x) << 1;
	if (h3 < h2)
		h3 = h2;
	
	leftx = x + (h1 >> 1) + 11;
	lefty = y + SBARHT - h1;
	start = h2 - h1;
	
	x += h2>>1;
	y += SBARHT - h2;
	len = h3 - h2;
	end = h4 - h3;
	
	if (h2 < h4)
	asm
	{
		move.l	D3, -(SP)
		JSR_WADDRESS
		move.l	#-64*2, D2
		moveq	#3, D1
		andi.w	#15, x
		move.l	#NNE_MASK, D0
		ror.l	x, D0
		asr.w	#1, len
		ble.s	@doend
		
		asl.w	#2, D2
		move.w	len, D3
		and.w	#3, len
		asr.w	#2, D3
		subq.w	#1, D3
		blt.s	@slower
		
	@fast	and.l	D0, (A0)
		and.l	D0, -64(A0)
		ror.l	#1, D0
		and.l	D0, -64*2(A0)
		and.l	D0, -64*3(A0)
		ror.l	#1, D0
		and.l	D0, -64*4(A0)
		and.l	D0, -64*5(A0)
		ror.l	#1, D0
		and.l	D0, -64*6(A0)
		and.l	D0, -64*7(A0)
		adda.l	D2, A0
		ror.l	#1, D0
		btst	D1, D0
		dbeq	D3, @fast
		bne.s	@slower
		swap	D0
		addq.w	#2, A0
		dbra	D3, @fast
		
	@slower	asr.l	#2, D2
		bra.s	@enter1
		
	@loop1	and.l	D0, (A0)
		and.l	D0, -64(A0)
		adda.l	D2, A0
		ror.l	#1, D0
		btst	D1, D0
	@enter1	dbeq	len, @loop1
		bne.s	@doend
		
		swap	D0
		addq.w	#2, A0
		dbra	len, @loop1
		bra.s	@leave		/* can't have end here */
		
	@doend	swap	D0
		move.w	end(A6), len
		asr.w	#1, len
		subq.w	#1, len
		blt.s	@leave
		
	@loop2	and.w	D0, (A0)
		and.w	D0, -64(A0)
		asr.w	#1, D0
		adda.l	D2, A0
		dbra	len, @loop2
	@leave
		move.l	(SP)+, D3
	}
	
	if (start <= 0)
		return;
	x = leftx;
	y = lefty;
	asm
	{
		JSR_WADDRESS
		move.w	#0x7FFF, D0
		lsr.w	x, D0
		move.w	start(A6), len
		asr.w	#1, len
		bra.s	@enter
	
	@loop3	and.w	D0, (A0)
		and.w	D0, -64(A0)
		lsr.w	#1, D0
		suba.w	#128, A0
	@enter	dbra	len, @loop3
	}
}


#define	NE_MASK	0xFFFE0000
#define	NE_VAL	0xC0000000

/*
	NE_Black:  draws black parts of a northeast line.
*/
void ne_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len;
	register long eor;
	int endline, end, h0, h1, h15, h2, h3, h4, startx, starty, startlen;
	register char *screen;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = line->h1;
	h4 = line->length+1;
	
	if (y - h1 >= VIEWHT)
		h1 = y - (VIEWHT-1);
	if (y < h4)
		h4 = y + 1;
	if (x + h1 < -14)
		h1 = -14-x;
	if (x + h4 > SCRWTH)
		h4 = SCRWTH - x;
	if (h1 > h4)
		h1 = h4;
	h3 = line->h2;
	if (h3 > h4)
		h3 = h4;
	h15 = h3;
	if (x + h15 > 0)
		h15 = -x;
	if (h15 < h1)
		h15 = h1;
	startlen = h15 - h1;
	if (x + h15 < 0)
		h15 = -x;
	if (h3 < h15)
		h3 = h15;
	h2 = h3;
	if (x + h2 > SCRWTH-15)
		h2 = (SCRWTH-15) - x;
	if (h2 < h15)
		h2 = h15;
	h0 = 0;
	if (x + h0 < 0)
		h0 = -x;
	if (y - h0 >= VIEWHT)
		h0 = y - (VIEWHT-1);
	
	y += SBARHT;
	
	startx = x + h1 + 14;
	starty = y - h1;
	
	len = h2 - h15;
	end = h3 - h2;
	endline = h4 - h3;
	
	if (h1 - h0 > 1)
		draw_neline(x+h0, y-h0, h1 - h0 - 1, L_UP);
	if (endline > 0)
		draw_neline(x+h3, y-h3, endline-1, L_UP);

	x += h15;
	y -= h15;
		
	eor = (background[(x+y)&1] & NE_MASK) ^ NE_VAL;
	
	if (len > 0 || end > 0)
	asm
	{
		FIND_WADDRESS(x,y)
		move.l	A0, screen
		moveq	#-64, D2
		
		andi.w	#15, x
		ror.l	x, eor
		subq.w	#1, len
		bge.s	@loop1
		swap	eor
		bra.s	@doend
		
	@loop1	eor.l	eor, (screen)
		adda.l	D2, screen
		ror.l	#1, eor
		dbcs	len, @loop1
		swap	eor
		addq.w	#2, screen
		subq.w	#1, len
		bge.s	@loop1
		tst.b	eor
		beq.s	@1
		subq.w	#2, screen
		bra.s	@doend
	@1	swap	eor
		
	@doend	move.w	end(A6), len
		subq.w	#1, len
		blt.s	@leave
		
	@loop2	eor.w	eor, (screen)
		lsr.w	#1, eor
		adda.l	D2, screen
		dbra	len, @loop2
	@leave	
	}
	x = startx;
	y = starty;
	len = startlen;
	if (len > 0)
		asm
		{
			JSR_WADDRESS
			
			move.w	#0x7FFF, D0
			asr.w	x, D0
			moveq	#64, D1
			bra.s	@enterlp
			
		@lp	and.w	D0, (A0)
			suba.l	D1, A0
			lsr.w	#1, D0
		@enterlp	dbra	len, @lp
		}
}


#define	ENE_VAL	0xF000
#define	ENE_MASK1	0x8000
#define	ENE_MASK2	0x01FFFFFF
/*
	ENE_Black:  draws black parts of a ENE line.
*/
void ene_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len;
	int h1, h2, h3, h4, end, endline, endlinex, endliney, length;
	void ene_white();
	
	ene_white(line, scrx, scry);
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h4 = line->length+1;
	
	if (x + h1 < 0)
		h1 = -x;
	if (y - (h1 >> 1) > VIEWHT)
		h1 = (y - VIEWHT) << 1;
	if (h1 & 1)
		h1++;
	if (x + h4 > SCRWTH)
		h4 = SCRWTH - x;
	if (y - (h4 >> 1) < 0)
		h4 = y << 1;
	if (h4 & 1)			/* can be optimized */
		h4--;
	if (h4 <= h1)
		return;
	h3 = line->h2;
	if (h3 > h4)
		h3 = h4;
	if (h3 & 1)
		h3--;
	if (h3 < h1)
		h3 = h1;
	h2 = h3;
	if (x + h2 >= SCRWTH-20)
		h2 = (SCRWTH-21) - x;
	if (h2 & 1)
		h2--;
	if (h2 < h1)
		h2 = h1;
	
	len = h2 - h1;
	end = h3 - h2;
	endline = h4 - h3;
	y += SBARHT;
	
	endlinex = x + h3 - 2;
	endliney = y - (h3>>1) + 1;
	if (endlinex < 0)
	{
		endlinex += 2;
		endliney--;
		endline -= 2;
	}
	
	x += h1;
	y -= (h1>>1) + 1;
	len >>= 1;
	len--;
	end >>= 1;
	
	if (len < 0)
		len = 0;
	asm {
		move.l	D3, -(SP)
		JSR_WADDRESS
		moveq	#64, D3
		
		cmp.w	#SCRWTH-16-3, x
		blt.s	@normal
		and.w	#31, x
		move.l	#ENE_VAL<<16, D0
		lsr.l	x, D0
		move.l	#ENE_MASK1<<16, D2
		asr.l	x, D2
		cmp.w	#16, x
		blt	@endst
		subq.w	#2, A0
		bra	@endst
		
	@normal	and.w	#15, x
		move.w	#ENE_MASK1, D1
		asr.w	x, D1
		move.l	#ENE_MASK2, D2
		lsr.l	x, D2
		move.w	#ENE_VAL, D0
		ror.w	x, D0
		bra.s	@enter1
		
	@loop1	and.w	D1, (A0)
		or.w	D0, (A0)
		and.l	D2, 2(A0)
		suba.l	D3, A0
		lsr.l	#2, D2
		asr.w	#2, D1
		ror.w	#2, D0
	@enter1	dbcs	len, @loop1
		subq.w	#1, len
		blt.s	@endstuff
		
		move.w	D0, D1
		and.w	#0xFF00, D1	/* get only overflow */
		
		or.b	D0, 1(A0)
		and.l	D2, 2(A0)
		or.w	D1, 2(A0)
		suba.l	D3, A0
		lsr.l	#2, D2
		ror.w	#2, D0
		asr.w	#2, D1
		subq.w	#1, len
		blt	@leave		/* "end" not poss. here */
		
		or.b	D0, 1(A0)
		and.l	D2, 2(A0)
		or.w	D1, 2(A0)
		suba.w	#62, A0
		lsr.l	#2, D2
		swap	D2
		not.w	D2
		ror.w	#2, D0
		dbra	len, @loop1	/* D1 is ready to go! */
		bra.s	@leave
		
	@endstuff	move.w	D1, D2
		swap	D2
		swap	D0
		clr.w	D0
		
	@endst	move.w	end(A6), len
		subq.w	#1, len
		blt.s	@leave
		
	@loop2	and.l	D2, (A0)
		or.l	D0, (A0)
		suba.l	D3, A0
		asr.l	#2, D2
		lsr.l	#2, D0
		dbra	len, @loop2
		
	@leave	move.l	(SP)+, D3
	}
	if (endline > 0)
		draw_eneline(endlinex, endliney, endline+1, L_UP);
}


/*
	ENE_White:  draws white parts of an ENE line.
*/
void ene_white(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len, h;
	long andval;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	
	len = line->length;
	
	if (x > 0)
		return;
	h = 0;
	if (x + h < -20)
		h = -20-x;
	if (y - (h>>1) > VIEWHT)
		h = (y - (VIEWHT-1)) << 1;
	if (h & 1)
		h++;
		
	len = line->length-12;
	if (len > -x)
		len = -x;
	if (len & 1)
		len++;
	if (y < len>>1)
		len = y<<1;
	
	len -= h;
	len >>= 1;
	
	y += SBARHT- (h>>1);
	x += h;
	andval = 0x7FFFFFFF >> (x + 20);
	x = 0;
	asm
	{
		JSR_WADDRESS
		
		move.l	andval, D0
		moveq	#64, D1
		tst.w	len
		blt.s	@leave
		
	@loop	and.l	D0, (A0)
		suba.l	D1, A0
		lsr.l	#2, D0
		dbra	len, @loop
	@leave
	}
}



/*
	East_Black:  draws black parts of an eastward line.
*/
void east_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	static long data[6] = {-1, -1, 0, 0, 0, 0};
	long *dataptr = data;
	register int x, y, len, height, *dp;
	int h1, h2, h3, h4;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h4 = line->length+1;
	height = 6;
	
	if (x + h1 < 0)
		h1 = -x;
	if (x + h4 > SCRWTH)
		h4 = SCRWTH - x;
	if (h1 >= h4)
		return;
	h2 = 16;
	if (h2 < h1)
		h2 = h1;
	else if (h2 > h4)
		h2 = h4;
	h3 = line->h2;
	if (h3 > line->length)
		h3 = line->length;
	if (h3 < h2)
		h3 = h2;
	if (h3 > h4)
		h3 = h4;
	if (y<0)
	{
		dataptr -= y;
		height += y;
		y = 0;
	}
	else if (y > VIEWHT - 6)
		height = VIEWHT - y;
	height--;
	if (height < 0)
		return;
	y += SBARHT;
	
	if (y + height >= SBARHT+5 && y < SCRHT)
	{
		if (h2 > h1)
			draw_eline(x+h1, y, h2 - h1 - 1, L_DN);
		if (h4 > h3)
			draw_eline(x+h3, y, h4 - h3 - 1, L_DN);
	}
	len = h3 - h2 - 1;
	
	if (len < 0)
		return;
	x += h2;
	
	asm
	{
		move.l	D3, -(SP)
		JSR_WADDRESS
		moveq	#64, D2
		
		andi.w	#15, x
		move.w	x, D0
		add.w	len, D0
		cmpi.w	#16, D0
		bge.s	@normal
		
		moveq	#-1, D1	/* deal with real short */
		lsr.w	#1, D1
		lsr.w	len, D1
		ror.w	x, D1
		
		bsr	@oneword
		bra	@leave
		
	@normal	moveq	#-1, D1
		lsr.w	x, D1
		not.w	D1
		cmp.w	#5, height
		beq	@quick
		bsr	@oneword
		
		addq.l	#2, A0
		sub.w	#15, len
		add.w	x, len
		moveq	#0, D0
		bra.s	@enter1
		
	@slowlp	move.l	A0, A1
		move.w	height, D1
		move.l	dataptr(A6), dp
	@inner	move.l	(dp)+, (A1)
		adda.l	D2, A1
		dbra	D1, @inner	
		
		addq.l	#4, A0
	@enter1	sub.w	#32, len
		bge.s	@slowlp
	
	@continue	add.w	#32, len
		moveq	#-1, D1
		lsr.l	len, D1
		swap	D1
		bsr	@oneword
		addq.l	#2, A0
		swap	D1
		bsr	@oneword
		bra	@leave
		
	@oneword	move.w	height, D3
		move.w	D1, D0
		not.w	D0
		move.l	A0, A1
		move.l	dataptr(A6), dp
		
	@ow_loop	tst.l	(dp)+
		bne.s	@pos
		and.w	D1, (A1)
		adda.l	D2, A1
	@enterow	dbra	D3, @ow_loop
		rts
	@pos	or.w	D0, (A1)
		adda.l	D2, A1
		dbra	D3, @ow_loop
		rts	
		
	@quick				/* no vert clipping */
		not.w	D1
		or.w	D1, (A0)
		or.w	D1, 64(A0)
		not.w	D1
		and.w	D1, 64*2(A0)
		and.w	D1, 64*3(A0)
		and.w	D1, 64*4(A0)
		and.w	D1, 64*5(A0)
		
		addq.l	#2, A0
		sub.w	#15, len
		add.w	x, len
		moveq	#0, D0
		moveq	#-1, D1
		bra.s	@enter2
		
	@quicklp	move.l	D1, (A0)
		move.l	D1, 64(A0)
		move.l	D0, 64*2(A0)
		move.l	D0, 64*3(A0)
		move.l	D0, 64*4(A0)
		move.l	D0, 64*5(A0)
		addq.l	#4, A0
	@enter2	sub.w	#32, len
		bge.s	@quicklp
		
		add.w	#32, len
		moveq	#-1, D0
		lsr.l	len, D0
		
		not.l	D0
		or.l	D0, (A0)
		or.l	D0, 64(A0)
		not.l	D0
		and.l	D0, 64*2(A0)
		and.l	D0, 64*3(A0)
		and.l	D0, 64*4(A0)
		and.l	D0, 64*5(A0)
		
	@leave	move.l	(SP)+, D3
	}
}


#define	ESE_MASK	0xFC000000
#define	ESE_VAL	0x3C000000

/*
	ESE_Black:  draws black parts of a ESE line.
*/
void ese_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y;
	register long eor1, eor2;
	int h1, h2, h3, h4, startx, starty;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h4 = line->length-1;
	
	if (x + h1 < 2)
		h1 = 2-x;
	if (y + (h1 >> 1) < 0)
		h1 = -y << 1;
	if (h1 & 1)
		h1++;
	if (x + h4 > SCRWTH-2)
		h4 = SCRWTH-2 - x;
	if (y + (h4 >> 1) > VIEWHT)
		h4 = (VIEWHT - y) << 1;
	if (h4 & 1)
		h4--;	/* ensure even */
	if (h4 <= h1)
		return;
	h2 = 12;
	if (h2 < h1)
		h2 = h1;
	if (h2 > h4)
		h2 = h4;
	h3 = line->length - 5;
	if (h3 > h4)
		h3 = h4;
	if (h3 < h2)
		h3 = h2;
	
	y += SBARHT;
	
	startx = x + h1;
	starty = y + (h1>>1);

	if (h3 < h4)
		eseline(x+h3, y + (h3>>1), h4 - h3);
	
	x += h2 - 2;
	y += h2>>1;
	
	eor1 = (background[(x+y)&1] & ESE_MASK) ^ ESE_VAL;
	eor2 = (background[(x+y+1)&1] & ESE_MASK) ^ ESE_VAL;
	
	if (h2 < h3)
		asm
		{
			JSR_WADDRESS
			move.w	x, D0
			andi.w	#15, D0
			lsr.l	D0, eor1
			lsr.l	D0, eor2
			lsr.l	#2, eor2
			move.w	h3(A6), D2
			sub.w	h2(A6), D2
			asr.w	#1, D2
			bra.s	@enterfa
			
		@fast	eor.l	eor1, (A0)
			eor.l	eor2, 64(A0)
			lsr.l	#4, eor1
			lsr.l	#4, eor2
			eor.l	eor1, 64*2(A0)
			eor.l	eor2, 64*3(A0)
			lsr.l	#4, eor1
			lsr.l	#4, eor2
			adda.w	#64*4, A0
			tst.b	eor2
			beq.s	@enterfa
			swap	eor1
			swap	eor2
			addq.w	#2, A0
		@enterfa	subq.w	#4, D2
			bge.s	@fast
			
			addq.w	#4, D2
			bra.s	@enter1
			
		@loop1	eor.l	eor1, (A0)
			subq.w	#1, D2
			blt.s	@out
			eor.l	eor2, 64(A0)
			adda.w	#128, A0
			lsr.l	#4, eor1
			lsr.l	#4, eor2
		@enter1	dbra	D2, @loop1
		@out
		}
			
	
	if (h1 < h2)
		eseline(startx, starty, h2 - h1);
}


eseline(x, y, len)
register int x, y, len;
{
	asm
	{
		FIND_WADDRESS(x,y)
		and.w	#15, x
		move.l	#0xF0000000, D0
		lsr.l	x, D0
		moveq	#64, D1
		asr.w	#1, len
		subq.w	#1, len
		blt.s	@leave
		
	@loop	or.l	D0, (A0)
		adda.w	D1, A0
		lsr.l	#2, D0
		dbra	len, @loop
	@leave
	}
}
		
		

#define	SE_MASK	0xF8000000
#define	SE_VAL	0xC0000000

/*
	SE_Black:  draws black parts of a southeast line.
*/
void se_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len;
	register long eor;
	int end, h1, h2, h3, h4, h5;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h5 = line->length+1;
	
	if (x + h1 < 0)
		h1 = -x;
	if (y + h1 < 0)
		h1 = -y;
	if (x + h5 > SCRWTH)
		h5 = SCRWTH - x;
	if (y + h5 > VIEWHT)
		h5 = VIEWHT - y;
	if (h1 >= h5)
		return;
	h4 = line->h2;
	if (h4 > h5)
		h4 = h5;
	if (h4 < h1)
		h4 = h1;
	h2 = line->h1;
	if (h2 < h1)
		h2 = h1;
	if (h2 > h4)
		h2 = h4;
	h3 = h4;
	if (x + h3 > SCRWTH-16)
		h3 = SCRWTH-16 - x;
	if (h3 < h2)
		h3 = h2;
	
	y += SBARHT;
	len = h3 - h2;
	end = h4 - h3;
	
	if (h2 > h1)		/* draw short black-only piece */
		draw_neline(x+h1, y+h1, h2 - h1 - 1, L_DN);
	if (h5 > h4)
		draw_neline(x+h4, y+h4, h5 - h4 - 1, L_DN);
	
	x += h2;
	y += h2;
	
	if (len <= 0 && end <= 0)
		return;
	
	eor = (background[(x+y)&1] & SE_MASK) ^ SE_VAL;
	
	asm
	{
		FIND_WADDRESS(x,y)
		moveq	#64, D2
		andi.w	#15, x
		ror.l	x, eor
		subq.w	#1, len
		bge.s	@loop1
		swap	eor
		bra.s	@doend
		
	@loop1	eor.l	eor, (A0)
		adda.l	D2, A0
		ror.l	#1, eor
		dbcs	len, @loop1
		swap	eor
		addq.w	#2, A0
		subq.w	#1, len
		bge.s	@loop1
		tst.b	eor
		bne.s	@1
		swap	eor
		bra.s	@doend
	@1	subq.w	#2, A0
		
	@doend	move.w	end(A6), len
		subq.w	#1, len
		blt.s	@leave
		
	@loop2	eor.w	eor, (A0)
		lsr.w	#1, eor
		adda.l	D2, A0
		dbra	len, @loop2
		
	@leave
	}
}


#define	SSE_MASK	0xFF000000
#define	SSE_VAL	0xC0000000

/*
	SSE_Black:  draws black parts of a SSE line.
*/
void sse_black(line, scrx, scry)
linerec *line;
int scrx, scry;
{
	register int x, y, len;
	long eor1, eor2;
	int start, end, startx, starty, startlen;
	int h, h1, h2, h3, h4, h5;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h5 = line->length+1;
	
	if (x + (h1>>1) < 0)
		h1 = -x << 1;
	if (y + h1 < 0)
		h1 = -y;
	if (h1 & 1)
		h1++;
	if (x + (h5>>1) > SCRWTH-1)
		h5 = (SCRWTH-1 - x) << 1;
	if (y + h5 > VIEWHT)
		h5 = VIEWHT - y;
	if (h1 > h5)
		h1 = h5;
	h2 = line->h1;
	if (h2 < h1)
		h2 = h1;
	if (h2 > h5)
		h2 = h5;
	h4 = line->h2;
	if (h4 < h1)
		h4 = h1;
	if (h4 > h5)
		h4 = h5;
	h3 = h4;
	if (x + (h3>>1) > SCRWTH-8)
	{
		h3 = (SCRWTH-8 - x) << 1;
		if (h3 & 1)
			h3--;
	}
	if (h3 < h2)
		h3 = h2;
	
	if (x >= 0)
		startlen = 0;
	else
	{
		h = line->h1;
		if (x + (h>>1) < -7)
			h = (-7-x) << 1;
		if (y + h < 0)
			h = -y;
		if (h & 1)
			h++;
		startlen = h1 - h;
		startx = x + (h >> 1) + 7;
		starty = y + SBARHT + h;
	}

	y += SBARHT;
	start = h2 - h1;
	len = h3 - h2;
	end = h4 - h3;
	
	if (start > 0)		/* draw short black-only piece */
		draw_nneline(x+(h1>>1), y+h1, start-1, L_DN);
	if (h5 - h4 > 1)
		draw_nneline(x+(h4>>1), y+h4, h5 - h4 - 1, L_DN);
	
	x += h2 >> 1;
	y += h2;
	
	eor1 = (background[(x+y)&1] & SSE_MASK) ^ SSE_VAL;
	eor2 = (background[1-(x+y)&1] & SSE_MASK) ^ SSE_VAL;
	
	asm
	{
		move.l	D3, -(SP)
		FIND_WADDRESS(x,y)
		move.l	#128, D2
		andi.w	#15, x
		move.l	eor1(A6), D0
		move.l	eor2(A6), D1
		ror.l	x, D0
		ror.l	x, D1
		subq.w	#1, len
		blt.s	@doend
		bra.s	@enterq
		
	@quick	eor.l	D0, (A0)
		eor.l	D1, 64(A0)
		ror.l	#1, D0
		ror.l	#1, D1
		eor.l	D1, 64*2(A0)
		eor.l	D0, 64*3(A0)
		ror.l	#1, D0
		ror.l	#1, D1
		adda.w	#64*4, A0
		tst.b	D1
		beq.s	@enterq
		swap	D0
		swap	D1
		addq.w	#2, A0
	@enterq	subq.w	#4, len
		bge.s	@quick
		addq.w	#4, len
		
	@loop1	eor.l	D0, (A0)
		subq.w	#1, len
		blt.s	@leave
		eor.l	D1, 64(A0)
		adda.l	D2, A0
		ror.l	#1, D0
		ror.l	#1, D1
		move.l	D0, D3
		move.l	D1, D0
		move.l	D3, D1
		tst.b	D1
		dbne	len, @loop1
		beq.s	@doend
		
		swap	D0
		swap	D1
		addq.w	#2, A0
		dbra	len, @loop1
		bra.s	@leave		/* can't have end here */
		
	@doend	swap	D0
		swap	D1
		move.w	end(A6), len
		subq.w	#1, len
		blt.s	@leave
		
	@loop2	eor.w	D0, (A0)
		lsr.w	#1, D0
		subq.w	#1, len
		blt.s	@leave
		eor.w	D1, 64(A0)
		lsr.w	#1, D1
		move.l	D0, D3
		move.l	D1, D0
		move.l	D3, D1
		adda.l	D2, A0
		dbra	len, @loop2
		
	@leave	move.l	(SP)+, D3
	}
	len = startlen;
	if (len > 0)
	{
		x = startx;
		y = starty;
		asm
		{
			JSR_WADDRESS
			move.w	#0x7FFF, D0
			lsr.w	x, D0
			asr.w	#1, len
			bra.s	@enterlp
			
		@lp	and.w	D0, (A0)
			and.w	D0, 64(A0)
			lsr.w	#1, D0
			adda.w	#128, A0
		@enterlp	dbra	len, @lp
		}
	}
}


#define	SOUTH_BLACK	0xC0000000L
#define	SOUTH_MASK	0xFFC00000L

void south_black(line, scrx, scry)
register linerec *line;
int scrx, scry;
{
	register int x, y, len;
	int h1, h2, h3, h4;
	register char *screen;
	long eor1, eor2;
	
	x = line->startx - scrx;
	y = line->starty - scry;
	h1 = 0;
	h4 = line->length + 1;
	
	if (y + h1 < 0)
		h1 = -y;
	if (y + h4 > VIEWHT)
		h4 = VIEWHT - y;
	if (h1 >= h4)
		return;
	h2 = line->h1;
	if (h2 < h1)
		h2 = h1;
	if (h2 > h4)
		h2 = h4;
	h3 = line->h2;
	if (h3 < h2)
		h3 = h2;
	if (h3 > h4)
		h3 = h4;
	
	y += SBARHT;
	if (x >= 0 && x < SCRWTH)
	{
		if (h2 > h1)
			draw_nline(x, y+h1, h2 - h1 - 1, L_DN);
		if (h4 > h3 + 1)
			draw_nline(x, y+h3, h4 - h3 - 1, L_DN);
	}
	y += h2;
	len = h3 - h2;
	if (len <= 0)
		return;
		
	eor1 = (background[(x+y) & 1] & SOUTH_MASK) ^ SOUTH_BLACK;
	eor2 = (background[(x+y+1) & 1] & SOUTH_MASK) ^ SOUTH_BLACK;
	
	asm
	{
		move.l	D3, -(SP)
		move.l	#64*4, D2
		
		JSR_WADDRESS
		movea.l	A0, screen
		
		move.l	eor1(A6), D0
		move.l	eor2(A6), D1
		move.w	x, D3
		and.w	#15, D3
		lsr.l	D3, D0
		lsr.l	D3, D1
		move.w	len, D3
		asr.w	#2, D3
		and.w	#3, len
		
		tst.w	x
		blt.s	@quick1
		cmpi.w	#SCRWTH-16, x
		bge.s	@quick2
		and.w	#15, x
		cmp.w	#6, x
		ble.s	@quick2
		
		bra.s	@enter
		
	@loop	eor.l	D0, (screen)
		eor.l	D1, 64(screen)
		eor.l	D0, 64*2(screen)
		eor.l	D1, 64*3(screen)
		adda.l	D2, screen
	@enter	dbf	D3, @loop
		asr.l	#1, D2
		bra.s	@entere
		
	@end	eor.l	D0, (screen)
		subq.w	#1, len
		blt.s	@out
		eor.l	D1, 64(screen)
		adda.l	D2, screen
	@entere	dbra	len, @end
	@out	bra	@leave
		
	@quick1	addq.w	#2, screen
		bra.s	@enterq
	@quick2	swap	D0
		swap	D1
		bra.s	@enterq
		
	@qloop	eor.w	D0, (screen)
		eor.w	D1, 64(screen)
		eor.w	D0, 64*2(screen)
		eor.w	D1, 64*3(screen)
		adda.l	D2, screen
	@enterq	dbf	D3, @qloop
		asr.l	#1, D2
		bra.s	@enterqe
		
	@qend	eor.w	D0, (screen)
		subq.w	#1, len
		blt.s	@leave
		eor.w	D1, 64(screen)
		adda.l	D2, screen
	@enterqe	dbra	len, @qend
		
	@leave	move.l	(SP)+, D3
	}
}



void (*black_routines[9]) () =
{	NULL,
	south_black,
	sse_black,
	se_black,
	ese_black,
	east_black,
	ene_black,
	ne_black,
	nne_black};
	


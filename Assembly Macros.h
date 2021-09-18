/*	A file of assembly language macros for Lightspeed C

	Originally created for Gravity Well by Randy Wilson

*/

long find_waddr(), find_baddr();
extern char *back_screen;

#define	dbra	dbf

	/*	conditional negation	*/

#define	NEGIFNEG(x, y)		\
		tst.w	y	\
		bgt.s	@_1	\
		neg.w	x	\
	@_1:


	/* finding address of starting dot on byte or word boundary */
	
#define	JSR_WADDRESS			\
		jsr	find_waddr		\
		move.l	D0, A0
			
#define	JSR_BADDRESS			\
		jsr	find_baddr		\
		move.l	D0, A0
			
	
	/* starting address in macro version, ~2 times as fast */
#define	FIND_WADDRESS(thex, they)		\
		movea.l	back_screen(A5), A0	\
		move.w	thex, D0		\
		asr.w	#3, D0		\
		bclr.l	#0, D0		\
		adda.w	D0, A0		\
		move.w	they, D0		\
		asl.w	#6, D0		\
		adda.w	D0, A0

#define	FIND_BADDRESS(thex, they)		\
		movea.l	back_screen(A5), A0	\
		move.w	thex, D0		\
		asr.w	#3, D0		\
		adda.w	D0, A0		\
		move.w	they, D0		\
		asl.w	#6, D0		\
		adda.w	D0, A0






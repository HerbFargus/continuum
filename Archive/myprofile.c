/*
  
	Profile and tracing package for LightspeedC�.

	(C) Copyright 1986 THINK Technologies, Inc.  All rights reserved.
	
	Modified by Randy Wilson, June 26, 1987.
	
	This file replaces both "profilehooks.c" and "profile.c".

*/


#include        <stdio.h>
#include		<VRetraceMgr.h>
#include		<OSUtil.h>

#define	CURRENTA5	0x904				/* holds correct A5 */
#define	NUMROUTINES	800			/* total # of routines to be profiled */
#define	STACKSIZE	200			/* depth of stack */


typedef struct
{
	char *name;			/* name of routine */
	long count;			/* number of times found in it */
} _Profile_entry;


static _Profile_entry proftable[NUMROUTINES];
static profnum = 0;

static long nonprofilecount;


typedef struct
{
	char *name, *address;		/* name & return address of routine */
} _Routine_entry;


static _Routine_entry routine_stack[STACKSIZE];
static int depth = -1;		/* holds stack of routines as they are called */
static int maxdepth = -1;	/* highest value of depth */

static int lock = false;	/* keep interrupts out of data while changing */


static VBLTask taskrec;


int pstreq( s1, s2 )
register unsigned char	*s1, *s2;
{
  register int	n = *s1;
  
  if ((n = *s1++) != *s2++)
    return( false );
  while (n-- > 0)
    if (*s1++ != *s2++)
      return( false );
  return( true );
}


/*
	Do_Profile:
		Called by vertical retrace manager.  Adds an entry to correct
		routine's entry in table.
*/
pascal void _do_profile()
{
	register int i;
	char *current, *saveA5;
	
	asm{
		move.l	A5, saveA5(A6)			; save outside A5
		move.l	CURRENTA5, A5			; get correct A5
	}
	if (!lock)						/* try to avoid interrupt bugs */
	{
		if (depth < 0)
			nonprofilecount++;
		else
		{
			current = routine_stack[depth].name;
			for (i=0; i<profnum; i++)
				if (pstreq(current, proftable[i].name))
				{
					proftable[i].count++;
					break;
				}
			if (i == profnum && profnum < NUMROUTINES)
			{
				proftable[profnum].name = current;
				proftable[profnum].count = 1;
				profnum++;
			}
		}
	}

	taskrec.vblCount = 1;
	
	asm {
			move.l	saveA5(A6), A5		; and A5
	}
}



/*
	Start_Profile:
		Called to start profiling.  Just puts task on the VBL queue and
			inits data structures.
*/
start_profile()
{
		taskrec.qType = vType;
		taskrec.vblAddr = (ProcPtr) _do_profile;
		taskrec.vblPhase = 0;
		taskrec.vblCount = 1;
		VInstall(&taskrec);
		
		profnum = 0;
		depth = -1;
		nonprofilecount = 0L;
}


/*
	End_Profile(filename)
		Called to end profiling and dump info into file FILENAME.
*/
end_profile(filename)
char *filename;
{
	register int i;
	long total, grandtotal;
	FILE *fp;
	
	VRemove(&taskrec);
	
	ins_sort();
	
	if ((fp = fopen(filename, "w")) == NULL)
	{
		SysBeep(10);
		return;
	}
	fprintf(fp, "Routine Name          Entries   Profile %%  Grand %%\n");
	fprintf(fp, "------------          -------   ---------  -------\n");
	
	for (i=0, total=1; i<profnum; i++)
		total += proftable[i].count;
	grandtotal = total + nonprofilecount;
	
	for (i=0; i<profnum; i++)
	{
		PtoCstr(proftable[i].name);
		fprintf(fp, "%-20s%9ld%10.1lf%10.1lf\n", 
				proftable[i].name, proftable[i].count,
				(double) (proftable[i].count*100)/total,
				(double) (proftable[i].count*100)/grandtotal);
		CtoPstr(proftable[i].name);
	}
	
	fprintf(fp, "%-20s%9ld%10.1lf%10.1lf\n", 
			"Unprofiled", nonprofilecount,
			(double) (nonprofilecount*100)/total,
			(double) (nonprofilecount*100)/grandtotal);

	fprintf(fp, "----------------------------------------------------\n");
	fprintf(fp, "%-20s%9ld%10.1lf\n", 
				"Total", total, 100.0);
	fprintf(fp, "%-20s%9ld%10.1lf%10.1lf\n", 
				"Grand Total", grandtotal,
				(double) (total*100)/grandtotal, 100.0);
	fprintf(fp, "\nGreatest depth: %d\n", maxdepth);
	fclose(fp);
}



ins_sort()
{
	register int i, j;
	_Profile_entry temp;
	
	for (i=1; i<profnum; i++)
		for (j=i; j>0 && proftable[j].count > proftable[j-1].count; j--)
		{
			temp = proftable[j];
			proftable[j] = proftable[j-1];
			proftable[j-1] = temp;
		}
}



char *__profile_exit()
{	
	return(routine_stack[depth--].address);
}

 
/*
 
 Handle the routine entry profiling.  Setup a stack frame.
 
 */
 
void __profile( unused, ret, name )
char **ret, *unused, *name;
{
	void _profile_exit_();
	
	if (depth >= STACKSIZE-1)			/* don't overrun stack */
		return;
	
	depth++;
	routine_stack[depth].address = *ret;
	*ret = (char *) _profile_exit_;
	routine_stack[depth].name = name;
	if (depth > maxdepth)
		maxdepth = depth;
}



/*
;
; Entry point for profiler.  Add another parameter to the stack which is
; a pointer to the return address of the caller of this function.
;
*/
_profile_()
{
	asm
	{
		move.w	#true, lock(A5)
		move.l	(a7)+,d0			; get current return address
		pea		4(a6)				; push pointer to return address
		move.l	d0,-(a7)			; push back the return address
		jsr		__profile			; call the C profiler
		move.l	(a7)+,d0			; get return address
		move.l	d0,(a7)				; write on top of stack and return
		move.w	#false, lock(A5)
	}
}

/*
;
; Exit point for profiler.  This calls the end of function timing routine
; and returns to the user.
;
*/

void _profile_exit_()
{
	asm
	{
		move.w	#true, lock(A5)
		move.l	d0, -(a7)			; save old function result
		jsr		__profile_exit		; call end of routine timer
		move.l	d0, a0				; save return address in A0
		move.l	(a7)+, d0			; retrieve old function result
		move.w	#false, lock(A5)
		jmp		(a0)				; then return
	}
}


/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
//almost same as getmem(){made changes to getmem()} except we have tpo get free mem blocl from process' private heap, thus use  mblock *memlist which is declared in proctab[] 
	STATWORD ps;
        struct  mblock  *p, *q, *leftover;

        disable(ps);
        if (nbytes==0 || proctab[currpid].vmemlist->mnext== (struct mblock *) NULL) {
                restore(ps);
                return( (WORD *)SYSERR);
        }
        nbytes = (unsigned int) roundmb(nbytes);
        for (q= proctab[currpid].vmemlist,p=proctab[currpid].vmemlist->mnext ;
             p != (struct mblock *) NULL ;
             q=p,p=p->mnext)
                if ( p->mlen == nbytes) {
                        q->mnext = p->mnext;
                        restore(ps);
                        return( (WORD *)p );
                } else if ( p->mlen > nbytes ) {
                        leftover = (struct mblock *)( (unsigned)p + nbytes );
                        q->mnext = leftover;
                        leftover->mnext = p->mnext;
                        leftover->mlen = p->mlen - nbytes;
                        restore(ps);
                        return( (WORD *)p );
                }
        restore(ps); 
        return( (WORD *)SYSERR );


}

/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
//  kprintf("xmmap - to be implemented!\n");
	STATWORD ps;
	disable(ps);
	if(virtpage>4096)
	{
		bsm_map(currpid,virtpage,source,npages);
		restore(ps);
		return OK;
	}
	restore(ps);
return SYSERR;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
//  kprintf("To be implemented!");
	STATWORD ps;
	disable(ps);
	if(virtpage>4096)
	{
		bsm_unmap(currpid,virtpage,1);
	//	write_cr3(proctab[currpid].pdbr*NBPG);
		restore(ps);
		return OK;
	}	  
	restore(ps);
return SYSERR;
}

/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	STATWORD(ps);
	disable(ps);
	int i;
	for(i=0;i<8;i++)
	{
		bsm_tab[i].bs_status=BSM_UNMAPPED;
		bsm_tab[i].bs_pid=-1;
		bsm_tab[i].bs_vpno=4096;
		bsm_tab[i].bs_npages=0;
		bsm_tab[i].bs_sem=-1;
		bsm_tab[i].bs_priv_heap=0;
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD(ps);
	disable(ps);
	int i;
	for(i=0;i<8;i++)
	{
	if(bsm_tab[i].bs_status==BSM_UNMAPPED)
	{
		*avail=i;
		restore(ps);
		return OK;
	}}
	kprintf("\nBacking Store cannot be allocated\n");
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD(ps);
	disable(ps);
	bsm_tab[i].bs_status=BSM_UNMAPPED;
        bsm_tab[i].bs_pid=-1;
        bsm_tab[i].bs_vpno=4096;
        bsm_tab[i].bs_npages=0;
        bsm_tab[i].bs_sem=-1;
        bsm_tab[i].bs_priv_heap=0;
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD(ps);
	disable(ps);
	int i;
	int pgno=vaddr/NBPG;
	int bslim=bsm_tab[i].bs_vpno+bsm_tab[i].bs_npages;
	for(i=0;i<8;i++)
	{
		if(bsm_tab[i].bs_pid==pid)
		{
			if(pgno>bslim||pgno<bsm_tab[i].bs_vpno)
			{//if pgno is greater than requested pages(npages) OR less than it's starting point return error.
				printf("Requested Page not in BS bound");
				restore(ps);    
				return SYSERR; 
			}
		*pageth=pgno-bsm_tab[i].bs_vpno;//page no.of that pid in store i 
		*store=i;
		restore(ps);
		return OK;
		}
	}
	restore(ps);  
	return SYSERR;   //If none of the pid is matched in the entire bsm_tab
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD(ps);
	disable(ps);
	if(source<0||source>8||npages<1||npages>256)
	{
		restore(ps);
		return SYSERR;
	
	}
	bsm_tab[source].bs_status=BSM_MAPPED;
        bsm_tab[source].bs_pid=pid;
        bsm_tab[source].bs_vpno=vpno;
        bsm_tab[source].bs_npages=npages;
        bsm_tab[source].bs_sem=1;
        bsm_tab[source].bs_priv_heap=0;
	
	proctab[currpid].vhpno=vpno;
	proctab[currpid].store=source;
        restore(ps);
        return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
	disable(ps);
/*
	if(source<0||source>8||npages<1||npages>256)
        {       
                restore(ps);
                return SYSERR;
        }
*/	
	int i = 0,bs_number,pageth;
	unsigned long vaddr = vpno*NBPG;

	while(i < NFRAMES){
		if(frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_PAGE)
  		{
			bsm_lookup(pid,vaddr,&bs_number,&pageth);
			write_bs( (i+NFRAMES)*NBPG, bs_number, pageth);
  		}
		i++;
	}

	bsm_tab[proctab[pid].store].bs_status=BSM_UNMAPPED;
	bsm_tab[proctab[pid].store].bs_pid=-1;
	bsm_tab[proctab[pid].store].bs_vpno=vpno;
	bsm_tab[proctab[pid].store].bs_npages=0;
	bsm_tab[proctab[pid].store].bs_priv_heap=0;
	bsm_tab[proctab[pid].store].bs_sem=-1;
	restore(ps);
	return OK;
}

/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
extern int page_replace_policy;
/*-------------------------------------------------------------------------
 * pfinn- paging fault ISR
 *-------------------------------------------------------------------------
 */
void initialize_pt(int,pt_t*);
int create_pt();
void initialize_pt(int i,pt_t *pt)
{
//	pt_t *pt;
	for(;i<NFRAMES;i++)
	{
		pt[i].pt_pres=0;
		pt[i].pt_write=0;
		pt[i].pt_user=0;
                pt[i].pt_pwt=0;
                pt[i].pt_pcd=0;
                pt[i].pt_acc=0;
                pt[i].pt_dirty=0;
                pt[i].pt_mbz=0;
                pt[i].pt_global=0;
                pt[i].pt_avail=0;
                pt[i].pt_base=0;			
	}
}
int create_pt()
{
	int i=0,frno,fraddr;
	pt_t *pgt;
	if(get_frm(&frno)==SYSERR)
	{
//		kill(currpid);
		return -1;
	}
	
	fraddr=(FRAME0+frno)*4096; //get the frame addr by multiplying the frame size with the offset from zero-th frame
	pgt=(pt_t*)fraddr;  // for this fraddr add page table members as attributes
	initialize_pt(i,pgt);
	
	return frno;
}
SYSCALL pfint()
{
	STATWORD ps;
	disable(ps);
	virt_addr_t *v_addr;
	int newpt,newfr,bs_id,vpt_offset,vpd_offset,pg_offset,present;
	long pdbr,vaddr;
	present=0;
	pt_t *pte;
	pd_t *pde;
	pdbr=proctab[currpid].pdbr;
	vaddr=read_cr2();//read virt addr from control reg 2
	v_addr=(virt_addr_t*)&vaddr;//add virt_addr_t members to the location od vaddr
	vpt_offset=v_addr->pt_offset;
	vpd_offset=v_addr->pd_offset;
	pde=pdbr+(vpd_offset*sizeof(pd_t));//page directory entry
//	pte=(pt_t*)(pde->base*4096 + vpt_offset(pt_t));
	if((pde->pd_pres)==0)
	{
	//if no page directory exists then create one and update it.
		newpt=create_pt();
		if(newpt==-1)
		{
			kill(currpid);	//kill the process which failed to get a frame which inturn couldn't create the page table
			restore(ps);
			return SYSERR;
		}
		pde->pd_pres=1;
                pde->pd_write=1;
                pde->pd_user=0;
                pde->pd_pwt=0;
                pde->pd_pcd=0;
                pde->pd_acc=0;
                pde->pd_mbz=0;
		pde->pd_fmb=0;
                pde->pd_global=0;
                pde->pd_avail=0;
                pde->pd_base=FRAME0+newpt;
	
	// mark the obtained frame as marked and update.
		frm_tab[newpt].fr_status=FRM_MAPPED;
		frm_tab[newpt].fr_pid=currpid;
		frm_tab[newpt].fr_type=FR_TBL;
	}
	//page table entry calcualtion using directory base and virt page offset(which was obtained using virt addr
	pte=(pt_t*)(pde->pd_base*4096 + vpt_offset*sizeof(pt_t));
	if(pte->pt_pres==0)	
{
	//  kprintf("calling get_frm\n");
		if(get_frm(&newfr)==SYSERR)
		{
			kill(currpid);
                        restore(ps);
                        return SYSERR;
		}
	 //  kprintf("end get_frm\n");
		pte->pt_pres=1;
		pte->pt_write=1;
		pte->pt_base=FRAME0+newfr;

	// map the new frame and update it.
		frm_tab[newfr].fr_status=FRM_MAPPED;
		frm_tab[newfr].fr_pid=currpid;
		frm_tab[newfr].fr_vpno=vaddr/4096;
		frm_tab[pde->pd_base-FRAME0].fr_refcnt++;
		frm_tab[newfr].fr_type=FR_PAGE;
	//	bsm_lookup(currpid,vaddr,&bs_id,&pg_offset);
	// assign a backing store
		if(bsm_lookup(currpid,vaddr,&bs_id,&pg_offset)==SYSERR)
		{
//			kill(currpid);
			restore(ps);
			return SYSERR;	
		}
	//fetch looked up BS and apply Second Chance page replacement policy
		read_bs((char*)((FRAME0+newfr)*4096),bs_id,pg_offset);
//		if(page_replace_policy==SC)
			insert_frame(newfr);
//		else if(page_replace_policy==AGING)
//			insert_frame(newfr);
	}
	write_cr3(pdbr);	//update pdbr
	restore(ps);
return OK;
}


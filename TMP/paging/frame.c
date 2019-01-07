/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
extern int page_replace_policy;
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
 // kprintf("To be implemented!\n");
	STATWORD(ps);
	disable(ps);
	int i;
	for(i=0;i<NFRAMES;i++)
	{
		frm_tab[i].fr_status=FRM_UNMAPPED;
		frm_tab[i].fr_pid=-1;
		frm_tab[i].fr_vpno=0;
		frm_tab[i].fr_refcnt=0;
		frm_tab[i].fr_type=FR_PAGE;
		frm_tab[i].fr_dirty=0;

	}
	restore(ps);
return OK;
}
void init_queue()
{
	int i;
	for(i=0;i<NFRAMES;i++)
	{	
		frmq[i].fid=i;//store NFRAMES
		frmq[i].age=0;
		frmq[i].next_frame=-1;
		frmq[i].prev_frame=-1;
	}
}
/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
//  kprintf("To be implemented!\n");
	STATWORD(ps);
	disable(ps);
	int i,frno=-1;
	for(i=0;i<NFRAMES;i++)
	{
		if(frm_tab[i].fr_status==FRM_UNMAPPED)
		{
			*avail=i;
			restore(ps);
			return OK;
		}
	}  
//default SC replace policy
//get a frame to free according to SC or aging  policy
	if(page_replace_policy==AGING)
		frno=get_AGE_frame();
	else
		frno=get_SC_frame();
	if(frno>=0)
	{
		free_frm(frno);// remove the selcted frame
		*avail=frno;
//		restore(ps); 
//		return OK;	
	}
	else
	{//kprintf("\nframe error.");
		restore(ps);
		return SYSERR;
	}
	restore(ps);
return OK;
}
int get_SC_frame()
{
	STATWORD ps;
	disable(ps);
	int frno=0,curr=0,next=-1,prev=-1,vpt_offset,vpd_offset;
	long pdbr,vaddr;
	pt_t *pte;
	pd_t *pde;
	virt_addr_t *v_addr;
	
	//loop until you get a frame
	while(curr!=-1)
	{
//	kprintf("curr :%d\n",curr);
		vaddr=frm_tab[curr].fr_vpno;
		v_addr=(virt_addr_t*)&vaddr;
		vpt_offset=v_addr->pt_offset;
		vpd_offset=v_addr->pd_offset;
		pdbr=proctab[currpid].pdbr;
		pde=pdbr+vpd_offset*sizeof(pd_t);
		pte=(pt_t*)(pde->pd_base*4096 + vpt_offset*sizeof(pt_t));
		frno=head;// in SC policy curr position starts from head of the queue.
//	kprintf("in frma\n");	
		//SC policy
		if(pte->pt_acc==1)
		{
//			kprintf("Pt acc\n");
			//page access bit is set, clear it move to the next page.
			pte->pt_acc=0;
			prev=curr;
			curr=frmq[curr].next_frame;
		}
		else
		{//kprintf("Pt acc=0");
			if(prev==-1)
			{//when head pt_acc is 0, remove it from the queue
				
				head=frmq[curr].next_frame;
				frmq[curr].next_frame=-1;
		//		frno=head;
				if(debug==1)
				{
					kprintf("\nFrame %d removed.",frno);
				}
//				kprintf("Frame no.:%d",frno);
				restore(ps);
				return frno; 
			}
			else
			{//after head is removed, remove the immediate next frame
				frmq[prev].next_frame=frmq[curr].next_frame;
				frmq[curr].next_frame=-1;
				
				if(debug==1)
                                {
                                        kprintf("\nFrame %d removed.",frno);
                                }
                                restore(ps);
                                return frno;
			}
		}	
	//	prev=curr;
	//	curr=frmq[curr].next_frame;
	}
	// if curr is -1, then it is available to replaced, as it can only be set to -1 in the previous loops when pt_acc is 0.
	head=frmq[curr].next_frame;
	frmq[curr].next_frame=-1;
	if(debug==1)
        {
		kprintf("\nFrame %d removed.",frno);
        }
        restore(ps);
        return frno;
}
int get_AGE_frame()
{
	STATWORD ps;
        disable(ps);
        int frno=0,curr,next=-1,prev=-1,vpt_offset,vpd_offset;
        long pdbr,vaddr;
        pt_t *pte;
        pd_t *pde;
        virt_addr_t *v_addr;

        //loop until you get a frame
	while(curr!=-1)
        {
//      kprintf("curr :%d\n",curr);
                vaddr=frm_tab[curr].fr_vpno;
                v_addr=(virt_addr_t*)&vaddr;
                vpt_offset=v_addr->pt_offset;
                vpd_offset=v_addr->pd_offset;
                pdbr=proctab[currpid].pdbr;
                pde=pdbr+vpd_offset*sizeof(pd_t);
                pte=(pt_t*)(pde->pd_base*4096 + vpt_offset*sizeof(pt_t));
                frno=head;
		frmq[curr].age=frmq[curr].age/2;
		
		if(pte->pt_acc==1)
                {
			int x=frmq[curr].age+128;
			if(x<255)
				frmq[curr].age=x;
			else
				frmq[curr].age=255;
		}
		// get the min aged node
		if(frmq[curr].age<frmq[frno].age)
			frno=curr;
		prev=curr;	
		curr=frmq[curr].next_frame;
		
	}
	if(debug==1)
	{
		kprintf("Aging Policy: Frame %d removed\n",frno);
	}
	restore(ps);	
	return frno;
}



/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

//  kprintf("To be implemented!\n");
	STATWORD ps;
        disable(ps);
        int bsid,pgno,pt_offset,pd_offset,pg_loc;
        long pdbr,vaddr;
        pt_t *pte;
        pd_t *pde;
        if(((i==FR_TBL)&&(i<FR_TBL))||((i==FR_TBL)&&(i<FR_DIR))||((i==FR_PAGE)&&(i<FR_PAGE)))
	{//invalid frame no handling
		kprintf("\nInvalid frame no. Frame %d cannot be freed",i);
		restore(ps);
		return SYSERR;
	}
	//clearing frames is claering pages, not tables and directories
	if(frm_tab[i].fr_type==FR_PAGE)
	{
		vaddr=frm_tab[i].fr_vpno;
		pdbr=proctab[frm_tab[i].fr_pid].pdbr;
		pt_offset=vaddr&1023;
		pd_offset=vaddr/NFRAMES;
		pde=pdbr+(pd_offset*sizeof(pd_t));
		pte=(pde->pd_base*NBPG) + (pt_offset*sizeof(pt_t));
		bsid=proctab[frm_tab[i].fr_pid].store;
		pgno=frm_tab[i].fr_vpno-proctab[frm_tab[i].fr_pid].vhpno;
		pg_loc=pde->pd_base-FRAME0;
		
		write_bs((i+FRAME0)*4096,bsid,pgno);
		
		pte->pt_pres=0;
		frm_tab[pg_loc].fr_refcnt=frm_tab[pg_loc].fr_refcnt-1;
		if((frm_tab[pg_loc].fr_refcnt)==0)
		{
			pde->pd_pres=0;
		
			frm_tab[pg_loc].fr_status=FRM_UNMAPPED;
			frm_tab[pg_loc].fr_pid=-1;
			frm_tab[pg_loc].fr_vpno=4096;
		//	frm_tab[pg_loc].fr_refcnt=frm_tab[pg_loc].fr_refcnt-1;
			frm_tab[pg_loc].fr_type=FR_PAGE;
		}
	}
	restore(ps);
return OK;
}

void insert_frame(int frno)
{
	STATWORD ps;
	disable(ps);
	int curr=head;
	int next=frmq[head].next_frame;
	
	if(head=-1)
	{//if only one element is the in the queue 
		head=frno;
		restore(ps);	
		return OK;
	}
	//if not find the last element
	while(next!=-1)
	{	
		curr=next;
		next=frmq[next].next_frame;
	}
	// insert after the last frame and link the new frame to the head of the queue.
	frmq[curr].next_frame=frno;
        frmq[frno].next_frame=-1; //mark it available

        restore(ps);
        return OK;
}

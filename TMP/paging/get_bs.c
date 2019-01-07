#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) 
{

  // requests a new mapping of npages with ID map_id 
//    kprintf("To be implemented!\n");
	STATWORD ps;
	disable(ps);
	if(bs_id<0||bs_id>7||npages<=0||npages>256)
	{
		restore(ps);
		return SYSERR;
	}
	if(bsm_tab[bs_id].bs_status==BSM_MAPPED)
	{
		if(bsm_tab[bs_id].bs_priv_heap==1)
		{	
			restore(ps);
			return SYSERR;
		}
		if(bsm_tab[bs_id].bs_pid==currpid||bsm_tab[bs_id].bs_pid!=currpid)
		{
	//		kprintf("\nin get_bs");
			bsm_tab[bs_id].bs_pid==currpid;
			bsm_tab[bs_id].bs_vpno=0;
			bsm_tab[bs_id].bs_npages=npages;
	//		restore(ps);
		}
	}
	if(bsm_tab[bs_id].bs_status==BSM_UNMAPPED)
	{//kprintf("\nin get_bs2");
		bsm_tab[bs_id].bs_status=BSM_MAPPED;
		bsm_tab[bs_id].bs_pid=currpid;
		bsm_tab[bs_id].bs_vpno=0;
		bsm_tab[bs_id].bs_npages=npages;
		bsm_tab[bs_id].bs_sem==-1;
		bsm_tab[bs_id].bs_priv_heap=0;
	//	restore(ps);
	}
//kprintf("\nin get_bs3");
restore(ps);
  return npages;

}

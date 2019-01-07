/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>

extern int debug;
extern int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */

//  kprintf("To be implemented!\n");
	STATWORD(ps);
	disable(ps);
	
	if(policy==SC||policy==AGING)
	{
	//	restore(ps);
		debug=1;
		page_replace_policy=policy;
		restore(ps);
		return OK;
	}
	restore(ps);
	return SYSERR;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

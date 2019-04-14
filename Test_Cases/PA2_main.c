/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
#define TPASSED 1
#define TFAILED 0

#define MYVADDR1	 0x40000000
#define MYVPNO1      0x40000
#define MYVADDR2     0x80000000
#define MYVPNO2      0x80000
#define MYBS1	1
#define MAX_BSTORE 8

#ifndef NBPG
#define NBPG 4096
#endif

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }

void proc_test1(int *ret) {
    char *addr0 = (char *)0x410000; 
	char *addr1 = (char*)0x40000000;
    int  i= ((unsigned long)addr1)>>12;

	*(addr0) = 'D';
	get_bs(MYBS1, 100);

	if (xmmap(i, MYBS1, 100) == SYSERR) {
	    *ret = TFAILED;
	    return;
	}

	for (i=0; i<16; i++){
	    *addr1 = 'A'+i;
	    addr1 += NBPG;
	}

	addr1 = (char*)0x40000000; 
	for (i=0; i<16; i++){
		if (*addr1 != 'A'+i){
			*ret = TFAILED;
			return;
		}
		addr1 += NBPG;
	}

	xmunmap(0x40000000>>12);
	release_bs(MYBS1);
	return;
}

void test1()
{
	int mypid;
    int ret = TPASSED;

	
    mypid = create(proc_test1, 2000, 20, "proc_test1", 1, &ret);
    resume(mypid);
    sleep(4);
    kill(mypid);
    if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*----------------------------------------------------------------*/

void proc_test2(int i,int j,int* ret,int s) {
	char *addr;
	int r;

	get_bs(i, j);

	r = xmmap(MYVPNO1, i, j);
	if (j<=50 && r == SYSERR){
		*ret = TFAILED;
	}
	if (j> 50 && r != SYSERR){
		*ret = TFAILED;
	}

	sleep(s);

	if (r != SYSERR) 
		xmunmap(MYVPNO1);

	release_bs(i);
	return;
}

void test2() {
	int pids[MAX_BSTORE];
	int mypid;
	int i,j;

	int ret = TPASSED;
	

	mypid = create(proc_test2, 2000, 20, "proc_test2", 4, 1,50,&ret,4);
	resume(mypid);
	sleep(2);
	for(i=1;i<=5;i++){
		pids[i] = create(proc_test2, 2000, 20, "proc_test2", 4, 1,i*20,&ret,0);
		resume(pids[i]);
	}
	sleep(3);
	kill(mypid);
	for(i=1;i<=5;i++){
		kill(pids[i]);
	}
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/

void proc1_test3(int i,int* ret) {
	char *addr;
	int bsize;
	int r;

	get_bs(i, 100);

	if (xmmap(MYVPNO1, i, 100) == SYSERR) {
	    *ret = TFAILED;
	    return 0;
	}
	sleep(4);
	xmunmap(MYVPNO1);
	release_bs(i);
	return;
}

void proc2_test3() {
	/*do nothing*/
	sleep(1);
	return;
}

void test3() {
	int pids[MAX_BSTORE];
	int mypid;
	int i,j;

	int ret = TPASSED;
	
	for(i=0;i < MAX_BSTORE;i++){
		pids[i] = create(proc1_test3, 2000, 20, "proc1_test3", 2, i,&ret);
		if (pids[i] == SYSERR){
			ret = TFAILED;
		}else{
			resume(pids[i]);
		}
	}
	sleep(1);
	mypid = vcreate(proc2_test3, 2000, 100, 20, "proc2_test3", 0, NULL);
	if (mypid != SYSERR)
		ret = TFAILED;

	for(i=0;i < MAX_BSTORE;i++){
		kill(pids[i]);
	}
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/

void proc1_test4(int* ret) {
	char *addr;
	int i;

	get_bs(MYBS1, 100);

	if (xmmap(MYVPNO1, MYBS1, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		*ret = TFAILED;
		sleep(3);
		return;
	}

	addr = (char*) MYVADDR1;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(6);

	for (i = 0; i < 26; i++) {
		if (*(addr + i * NBPG) != 'a'+i){
			*ret = TFAILED;
			break;
		}
	}


	xmunmap(MYVPNO1);
	release_bs(MYBS1);
	return;
}

void proc2_test4(int *ret) {
	char *addr;
	int i;

	get_bs(MYBS1, 100);

	if (xmmap(MYVPNO2, MYBS1, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		*ret = TFAILED;
		sleep(3);
		return;
	}

	addr = (char*) MYVADDR2;

	for (i = 0; i < 26; i++) {
		if (*(addr + i * NBPG) != 'A'+i){
			*ret = TFAILED;
			break;
		}
	}

	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'a' + i;
	}

	xmunmap(MYVPNO2);
	release_bs(MYBS1);
	return;
}

void test4() {
	int pid1;
	int pid2;
	int ret = TPASSED;

	pid1 = create(proc1_test4, 2000, 20, "proc1_test4", 1, &ret);
	pid2 = create(proc2_test4, 2000, 20, "proc2_test4", 1, &ret);

	resume(pid1);
	sleep(3);
	resume(pid2);

	sleep(10);
	kill(pid1);
	kill(pid2);
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/

void proc1_test5(int* ret) {
	int *x;
	int *y;
	int *z;

	x = vgetmem(1024);
	if ((x == NULL) || (x < 0x1000000)) {
		*ret = TFAILED;
    }
	if (x == NULL)
		return;

	*x = 100;
	*(x + 1) = 101;

	if ((*x != 100) || (*(x+1) != 101))
		*ret = TFAILED;
	vfreemem(x, 1024);

	x = vgetmem((256 + 1) * NBPG); //try to acquire a space that is bigger than size of one backing store
	if (x != SYSERR) {
		*ret = TFAILED;
    }

	x = vgetmem(50*NBPG);
	y = vgetmem(50*NBPG);
	z = vgetmem(50*NBPG);
	if ((x == SYSERR) || (y == SYSERR) || (z != SYSERR)){
		*ret = TFAILED;
		if (x != NULL) vfreemem(x, 50*NBPG);
		if (y != NULL) vfreemem(y, 50*NBPG);
		if (z != NULL) vfreemem(z, 50*NBPG);
		return;
	}
	vfreemem(y, 50*NBPG);
	z = vgetmem(50*NBPG);
	if (z == SYSERR){
		*ret = TFAILED;
	}
	if (x != NULL) vfreemem(x, 50*NBPG);
	if (z != NULL) vfreemem(z, 50*NBPG);
	return;


}

void test5() {
	int pid1;
	int ret = TPASSED;

	pid1 = vcreate(proc1_test5, 2000, 100, 20, "proc1_test5", 1, &ret);
;
	resume(pid1);
	sleep(3);
	kill(pid1);
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/

void proc1_test6(int *ret) {

    char *vaddr, *addr0, *addr_lastframe, *addr_last;
	int i, j;
	int tempaddr;
    int addrs[1200];

    int maxpage = (NFRAMES - (5 + 1 + 1 + 1));

	int vaddr_beg = 0x40000000;
	int vpno;

	for(i = 0; i < MAX_BSTORE; i++){
		tempaddr = vaddr_beg + 127 * NBPG * i;
		vaddr = (char *) tempaddr;
		vpno = tempaddr >> 12;
		get_bs(i, 127);
		if (xmmap(vpno, i, 127) == SYSERR) {
			*ret = TFAILED;
			kprintf("xmmap call failed\n");
			sleep(3);
			return;
		}

		for (j = 0; j < 127; j++) {
			*(vaddr + j * NBPG) = 'A' + i;
		}

		for (j = 0; j < 127; j++) {
			if (*(vaddr + j * NBPG) != 'A'+i){
				*ret = TFAILED;
				break;
			}
		}
		xmunmap(vpno);
		release_bs(i);
	}


	return;
}

void test6(){
	int pid1;
	int ret = TPASSED;

	pid1 = create(proc1_test6, 2000, 50, "proc1_test6",1,&ret);

	resume(pid1);
	sleep(4);
	kill(pid1);
	if (ret != TPASSED)
		kprintf("\tFAILED!\n");
	else
		kprintf("\tPASSED!\n");
}

/*-------------------------------------------------------------------------------------*/
void test_func7()
{
		int PAGE0 = 0x40000;
		int i,j,temp;
		int addrs[1200];
		int cnt = 0;
		//can go up to  (NFRAMES - 5 frames for null prc - 1pd for main - 1pd + 1pt frames for this proc)
		//frame for pages will be from 1032-2047
		int maxpage = (NFRAMES - (5 + 1 + 1 + 1));
        //int maxpage = (NFRAMES - 25);


		for (i=0;i<=maxpage/150;i++){
				if(get_bs(i,150) == SYSERR)
				{
						kprintf("get_bs call failed \n");
						return;
				}
				if (xmmap(PAGE0+i*150, i, 150) == SYSERR) {
						kprintf("xmmap call failed\n");
						return;
				}
				for(j=0;j < 150;j++)
				{
						//store the virtual addresses
						addrs[cnt++] = (PAGE0+(i*150) + j) << 12;
				}
		}

		/* all of these should generate page fault, no page replacement yet
		   acquire all free frames, starting from 1032 to 2047, lower frames are acquired first
		   */
		for(i=0; i < maxpage; i++)
		{
				*((int *)addrs[i]) = i + 1;
		}

		//trigger page replacement, this should clear all access bits of all pages
		//expected output: frame 1032 will be swapped out
		kprintf("\n\t 7.1 Expected replaced frame: 1032\n\t");
		*((int *)addrs[maxpage]) = maxpage + 1;

		for(i=1; i <= maxpage; i++)
		{

				if ((i != 600) && (i != 800))  //reset access bits of all pages except these
						*((int *)addrs[i])= i+1;

		}
		//Expected page to be swapped: 1032+600 = 1632
		kprintf("\n\t 7.2 Expected replaced frame: 1632\n\t");
		*((int *)addrs[maxpage+1]) = maxpage + 2;
		temp = *((int *)addrs[maxpage+1]);
		if (temp != maxpage +2)
			kprintf("\tFAILED!\n");

		kprintf("\n\t 7.3 Expected replaced frame: 1832\n\t");
		*((int *)addrs[maxpage+2]) = maxpage + 3;
		temp = *((int *)addrs[maxpage+2]);
		if (temp != maxpage +3)
			kprintf("\tFAILED!\n");


		for (i=0;i<=maxpage/150;i++){
				xmunmap(PAGE0+(i*150));
				release_bs(i);
		}

}
void test7(){
	int pid1;
	int ret = TPASSED;

	
	srpolicy(SC);
	pid1 = create(test_func7, 2000, 20, "test_func7", 0, NULL);

	resume(pid1);
	sleep(10);
	kill(pid1);


	kprintf("\n\t Finished! Check error and replaced frames\n");
}


int main() {
    int i, s;
    char buf[8];

	kprintf("\n\nHello World, Xinu lives\n\n");

	kprintf("\nTest 1: Test xmmap.\n");
	test1();
	kprintf("\nTest 2: Test backing store operations\n");
	test2();
	kprintf("\nTest 3: Test private heap\n");
	test3();
	kprintf("\nTest 4: Test shared backing store\n");
	test4();
	kprintf("\nTest 5: Test vgetmem/vfreemem function\n"); 
	test5();
	kprintf("\nTest 6: Stress testing\n");
	test6();
	kprintf("\nTest 7: Test SC page replacement policy\n");
	test7();

    shutdown();
}

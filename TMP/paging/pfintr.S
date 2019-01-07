/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
	popl pferrcode	//store the error code
	pushfl	//store flag register
	cli	//disable th einterrupts
	pushal	//save all gprs'
	call pfint //call pf handler
	popal	//restore all gprs'
	popfl	//restore flag registers
	iret

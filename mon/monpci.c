/* pci.c
 *
 * Benjamin A. Kuperman
 *
 * Some information here is modeled after that seen in the linux
 * source tree.
 */

/*
#include <conf.h>
#include <kernel.h>
*/
#include <mon/monnetwork.h>
#include <mon/mon3com.h>
#include <mon/monpci.h>
#include <mon/monitor.h>
#include <stdio.h>

/*
#define DEBUG
*/

/*------------------------------------------------------------------------

	      ####   #        ####   #####     ##    #        ####
	     #    #  #       #    #  #    #   #  #   #       #
	     #       #       #    #  #####   #    #  #        ####
	     #  ###  #       #    #  #    #  ######  #            #
	     #    #  #       #    #  #    #  #    #  #       #    #
	      ####   ######   ####   #####   #    #  ######   ####

 *------------------------------------------------------------------------
 */

static unsigned long mon_bios32_service(unsigned long service);
static bios32	*mon_bios32ptr;	/* pointer to the BIOS 32 structure */

/* from linux */
static unsigned long mon_bios32_entry = 0;
static struct {
    unsigned long address;
    unsigned short segment;
} mon_bios32_indirect = { 0, 0 + 5*0x08 /* 0x10  KERNEL_CS */ };

static long mon_pcibios_entry = 0;
static struct {
    unsigned long address;
    unsigned short segment;
} mon_pci_indirect = { 0, 0+5*0x08 /* 0x10  KERNEL_CS */ };

/*------------------------------------------------------------------------
 * pci_init - initialize the PCI bios structures and such
 *------------------------------------------------------------------------
 */
int mon_pci_init(void)
{
    if (OK == mon_pcibios_init()) {
	kprintf("mon_pci_init: PCI BIOS initialization ");
	kprintf("succeeded\n");
	return OK;
    } else {
	kprintf("mon_pci_init: PCI BIOS initialization ");
	kprintf("failed\n");
	return SYSERR;
    }
}

/*------------------------------------------------------------------------
 * pcibios_init
 *------------------------------------------------------------------------
 */
int mon_pcibios_init(void)
{
    unsigned char   *ptr;
    bios32	    *bptr;
    int		    i,length;
    unsigned char   cksum;

    /* search for it */
    for (bptr = (bios32 *)BIOS32_MIN_ADDR; 
	 bptr <= (bios32 *)BIOS32_MAX_ADDR; bptr++) {
	/* check for signature first */
	if (BIOS32_SIGNATURE != bptr->signature)
	    continue;
	/* now check the checksum */
	cksum=0;
	length = bptr->length * BIOS32_UNITS;
	ptr=(char *)bptr;
	for (i=0; i<length; i++) {
	    cksum += ptr[i];
	}
	if (0 != cksum) {
	    kprintf("mon_pcibios_init: invalid checksum at 0x%08lx\n",bptr);
	    continue;
	}
	/* verify the revision */
	if (0 != bptr->revision) {
	    kprintf("mon_pcibios_init: invalid revision %d at 0x%08lx\n",
		    bptr->revision,bptr);
	    continue;
	}
	/* If I am here, then all tests were valid */
	mon_bios32ptr = bptr;		/* set the global reference */


	mon_bios32_entry = mon_bios32ptr->bios32_entry;
	mon_bios32_indirect.address = mon_bios32_entry;

	if (mon_check_pcibios()) {
	    return OK;
	} else {
	    return SYSERR;
	}
    }

    /* If I am here, nothing was found */
    kprintf("mon_pcibios_init: no BIOS32 Service Directory was found\n");
    return SYSERR;

}


/*------------------------------------------------------------------------
 * mon_check_pcibios - Check for a PCI BIOS
 *------------------------------------------------------------------------
 */
int mon_check_pcibios(void)
{
    unsigned long signature;                                                        unsigned char present_status;
    unsigned char major_revision;
    unsigned char minor_revision;
    unsigned long flags;
    int pack;

    if ((mon_pcibios_entry = mon_bios32_service(PCI_SERVICE))) {
	mon_pci_indirect.address = mon_pcibios_entry;
	save_flags(flags);
	cli();
	__asm__(  "lcall *(%%edi)\n\t"
		  "jc 1f\n\t"
		  "xorb %%ah, %%ah\n"
		  "1:\tshl $8, %%eax\n\t"
		  "movw %%bx, %%ax"
		: "=d" (signature),
		  "=a" (pack)
		: "1" (PCIBIOS_PCI_BIOS_PRESENT),
		  "D" (&mon_pci_indirect)
		: "bx", "cx");
	restore_flags(flags);

	present_status = (pack >> 16) & 0xff;
	major_revision = (pack >> 8) & 0xff;
	minor_revision = pack & 0xff;
	if (present_status || (signature != PCI_SIGNATURE)) {
	    kprintf ("mon_pcibios_init : %s : BIOS32 Service Directory says PCI BIOS is present,\n"
		    "       but PCI_BIOS_PRESENT subfunction fails with present status of 0x%x\n"
		    "       and signature of 0x%08lx (%c%c%c%c).\n",
		    (signature == PCI_SIGNATURE) ?  "WARNING" : "ERROR",
		    present_status, signature,
		    (char) (signature >>  0), (char) (signature >>  8),
		    (char) (signature >> 16), (char) (signature >> 24));

	    if (signature != PCI_SIGNATURE)
		mon_pcibios_entry = 0;
	}
	if (mon_pcibios_entry) {
	    kprintf ("pcibios_init : PCI BIOS revision %x.%02x entry at 0x%lx\n",
		    major_revision, minor_revision, mon_pcibios_entry);
	    return 1;
	}
    }
    return 0;
}

static unsigned long mon_bios32_service(unsigned long service)
{
    unsigned char return_code;      /* %al */
    unsigned long address;          /* %ebx */
    unsigned long length;           /* %ecx */
    unsigned long entry;            /* %edx */
    unsigned long flags;

    /* According to the PCI BIOS specification, the following needs to
     * take place before the "CALL FAR":
     *
     * 1. The CS code segment selector and the DS data segment
     * selector must be set up to encompass the physical page holding
     * the entry point *as well as* the immediately following physical
     * page.
     *
     * 2. Them must also have the same base.
     *
     * 3. Platform BIOS writers must assume that CS is execute-only
     * and DS is read-only.
     *
     * 4. The SS stack segment selectore must provide at least 1K of
     * stack space.
     */

    save_flags(flags); 
    cli();
    asm(    "nop\n"
	    "nop\n"
	    "lcall *(%%edi)\n"
	    : "=a" (return_code),
	    "=b" (address),
	    "=c" (length),
	    "=d" (entry)
	    : "0" (service),
	    "1" (0),
	    "D" (&mon_bios32_indirect));
    restore_flags(flags);

    switch (return_code) {
	case 0:
	    return address + entry;
	case 0x80:      /* Not present */
	    kprintf("mon_bios32_service(0x%lx) : not present\n", service);
	    return 0;
	default: /* Shouldn't happen */
	    kprintf("mon_bios32_service(0x%lx) : returned unexpected value 0x%x\n",
		    service, return_code);
	    return 0;
    }
}


/*------------------------------------------------------------------------
 * find_pci_device - find a specified PCI device
 *------------------------------------------------------------------------
 */
int mon_find_pci_device(int deviceID, int vendorID, int index)
{
    unsigned long retval, info, error;
    int busNum, deviceNum, functionNum;

    /*
     * Returns the location of a PCI device based on the Device ID
     * and Vendor ID.
     */
#ifdef DEBUG
    kprintf("mon_find_pci_device: vendor=0x%x device=0x%x index=%d\n",
	    vendorID, deviceID, index);
#endif
    /*
     * Entry
     * -----
     * AH  - PCI_FUNCTION_CODE
     * AL  - FIND_PCI_DEVICE
     * CX  - Device ID
     * DX  - Vendor ID
     * SI  - Index (0..N)
     *
     * Exit
     * ----
     * BH  - Bus Number (0-255)
     * BL  - Dev Num (upper 5 bits), Function Number (lower 3 bits)
     * AH  - Return Code
     * CF  - Completion status: set=error, cleared=success
     */
/*
    save_flags(flags);
    cli();
*/
    asm(    "lcall *(%%edi)\n"	    /* call the function */
#if 0
	    /* Really need to check the carry flag here */
#else
	    "xor %%ecx, %%ecx\n"    /* placeholding */
#endif
	   :"=a" (retval),
	    "=b" (info),
	    "=c" (error)
	   :"0" (PCIBIOS_FIND_PCI_DEVICE),
	    "2" (deviceID),
	    "d" (vendorID),
	    "S" (index),
	    "D" (&mon_pci_indirect));
/*
    restore_flags(flags);
*/
    /* set up the return values */
    busNum	    = 0xff & (info >> 8);
    deviceNum	    = 0x07 & info;	    /* lower 3 bits */
    functionNum	    = 0x1f & (info >> 3);   /* next 5 bits */
#ifdef DEBUG
    kprintf("mon_find_pci_device: error=0x%x bus=%d dev=0x%x fun=0x%x (info=0x%x)\n",
	    error, busNum, deviceNum, functionNum, info);
    kprintf("retval = 0x%lx\n",0xff & (retval>>8));
#endif
    if (PCIBIOS_SUCCESSFUL == (0xff & (retval>>8))) {
	/* Florian seems happy with just this */
	mon_dev_eth.pcidev = info;
	return OK;
    } else
	return SYSERR;

}


int mon_pci_bios_read_config_byte(unsigned long dev, int where, unsigned char *value)
{
	unsigned long ret;

	__asm__("lcall *(%%esi); cld\n\t"
		"jc 1f\n\t"
		"xorb %%ah, %%ah\n"
		"1:"
		: "=c" (*value),
		  "=a" (ret)
		: "1" (PCIBIOS_READ_CONFIG_BYTE),
		  "b" (dev),
		  "D" ((long) where),
		  "S" (&mon_pci_indirect));
	return (int) (ret & 0xff00) >> 8;
}

int mon_pci_bios_read_config_word(unsigned long dev, int where, unsigned short *value)
{
	unsigned long ret;

	__asm__("lcall *(%%esi); cld\n\t"
		"jc 1f\n\t"
		"xorb %%ah, %%ah\n"
		"1:"
		: "=c" (*value),
		  "=a" (ret)
		: "1" (PCIBIOS_READ_CONFIG_WORD),
		  "b" (dev),
		  "D" ((long) where),
		  "S" (&mon_pci_indirect));
	return (int) (ret & 0xff00) >> 8;
}

int mon_pci_bios_read_config_dword(unsigned long dev, int where, unsigned long *value)
{
	unsigned long ret;

	__asm__("lcall *(%%esi); cld\n\t"
		"jc 1f\n\t"
		"xorb %%ah, %%ah\n"
		"1:"
		: "=c" (*value),
		  "=a" (ret)
		: "1" (PCIBIOS_READ_CONFIG_DWORD),
		  "b" (dev),
		  "D" ((long) where),
		  "S" (&mon_pci_indirect));
	return (int) (ret & 0xff00) >> 8;
}

int mon_pci_bios_write_config_byte(unsigned long dev, int where, unsigned char value)
{
	unsigned long ret;

	__asm__("lcall *(%%esi); cld\n\t"
		"jc 1f\n\t"
		"xorb %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_BYTE),
		  "c" (value),
		  "b" (dev),
		  "D" ((long) where),
		  "S" (&mon_pci_indirect));
	return (int) (ret & 0xff00) >> 8;
}

int mon_pci_bios_write_config_word(unsigned long dev, int where, unsigned short value)
{
	unsigned long ret;

	__asm__("lcall *(%%esi); cld\n\t"
		"jc 1f\n\t"
		"xorb %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_WORD),
		  "c" (value),
		  "b" (dev),
		  "D" ((long) where),
		  "S" (&mon_pci_indirect));
	return (int) (ret & 0xff00) >> 8;
}

int mon_pci_bios_write_config_dword(unsigned long dev, int where, unsigned long value)
{
	unsigned long ret;

	__asm__("lcall *(%%esi); cld\n\t"
		"jc 1f\n\t"
		"xorb %%ah, %%ah\n"
		"1:"
		: "=a" (ret)
		: "0" (PCIBIOS_WRITE_CONFIG_DWORD),
		  "c" (value),
		  "b" (dev),
		  "D" ((long) where),
		  "S" (&mon_pci_indirect));
	return (int) (ret & 0xff00) >> 8;
}

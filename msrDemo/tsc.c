/**
 * @file tsc.c
 * @author Swapnil Raykar (swap612@gmail.com)
 * @brief Kernel Module for reading timestamp counter MSR
 * @version 0.1
 * @date 2019-07-30
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/init.h>   /* Needed for the macros */

// Module init function
static int __init msrDemo_init(void)
{
	printk(KERN_INFO "Initialize MSR Demo\n");

	uint64_t expFlag, modFlag, setbitmask = (1LL << 21); // make 1 at bit21 position
	asm volatile("pushfq \n\t"
				 "popq %%rax \n\t"
				 "or %2, %%rax \n\t"
				 "mov %%rax, %0 \n\t"
				 "pushq %%rax \n\t"
				 "popfq \n\t"
				 "pushfq \n\t "
				 "popq %1 \n\t"
				 : "=g"(expFlag), "=g"(modFlag)
				 : "r"(setbitmask)
				 : "%rax");
	// compare expected and modified values of flag
	if (modFlag == expFlag)
	{
		// CPUID suported, check for MSR
		printk(KERN_INFO "CPUID Supported\n");
		uint64_t eax, ebx, ecx, edx;
		eax = 0x1, ecx = 0x0;
		asm volatile("cpuid"
					 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
					 : "a"(eax), "c"(ecx)
					 : "memory");
		if ((edx >> 5) & 1)
		{
			// MSR supported now read/write msr
			printk(KERN_INFO "MSR Supported\n");
			uint32_t lo, hi;
			uint64_t msr_id = 0x10; //dec(16) TSC  ;
			// executing rdmsr instruction`
			asm volatile(
				"rdmsr"
				: "=a"(lo), "=d"(hi)
				: "c"(msr_id));
			uint64_t result = (((uint64_t)hi << 32) | lo);
			printk(KERN_INFO "TSC MSR value is %lld\n", result);
		}
		else
			printk(KERN_INFO "MSR Not Supported\n");
	}
	else
		printk(KERN_INFO "CPUID Not supported \n");

	return 0;
}

// Exit function - called when module removed
static void __exit msrDemo_exit(void)
{
	printk(KERN_INFO "Cleanup for MSR Demo\n");
}

// Register the Module init and exit
module_init(msrDemo_init);
module_exit(msrDemo_exit);

// Set the Module Metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Swapnil Raykar <swap612@gmail.com>");
MODULE_DESCRIPTION("MSR demo Module ");

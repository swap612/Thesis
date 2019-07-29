/**
 * @file checkIntelRDT.cpp
 * @author Swapnil Raykar (swap612@gmail.com)
 * @brief Task 5: Write a CPP program to check Intel RDT Monitoring and Allocation Capabilities
 * @version 0.2
 * @date 2019-07-29
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <iostream>
#include <cstring>
using namespace std;

#define CPUID_BIT 21

/**
 * @brief Return if CPUID is supported
 * 
 * @return true 
 * @return false 
 */
bool static inline checkCpuidSupport()
{
    // expFlag - expected value of flag, modflag - modified flag
    uint64_t expFlag, modFlag, setbitmask = (1LL << CPUID_BIT); // make 1 at bit21 position
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
    // cout <<expFlag << modFlag ;
    // compare expected and modified values of flag
    if (modFlag == expFlag)
        return true;
    return false;
}

/**
 * @brief Return RDT Monitoring or Allocation Supported. Print the supported capabilities
 * 
 * @return true 
 * @return false 
 */
bool static inline checkRDTCapability()
{
    uint64_t eax, ebx, ecx, edx;
    eax = 0x07, ecx = 0; // input EAX= 0x07, ECX= 0
    ebx = 0, edx = 0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax), "c"(ecx)
                 : "memory");
    // cout << "EAX: " << eax << " EBX: " << ebx << " ECX: " << ecx << " EDX: " << edx << "\n";
    // Check RDT-M
    if ((ebx >> 12) & 1)
        cout << "RDT Monitoring Capability Supported\n";
    else
        cout << "RDT Monitoring Capability Not Supported\n";

    // check RDT-A
    if ((ebx >> 15) & 1)
        cout << "RDT Allocation Capability Supported\n";
    else
        cout << "RDT Allocation Capability Not Supported\n";

    if (((ebx >> 12) & 1) || ((ebx >> 15) & 1))
        return true;
    return false;
}

/**
 * @brief Print specific functionalities supported by RDT allocation
 * 
 */
void static inline checkRDTAllocation()
{
    uint64_t eax, ebx, ecx, edx;
    eax = 0x10, ecx = 0x0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax), "c"(ecx)
                 : "memory");
    // cout << "EAX: " << eax << " EBX: " << ebx << " ECX: " << ecx << " EDX: " << edx << "\n";
    // cout << ((ebx >> 1) & 1) << ((ebx >> 2) & 1) << ((ebx >> 3) & 1) << "\n";

    // check RDT Allocation
    if ((ebx >> 1) & 1)
        cout << "L3 CAT Supported\n";
    else
        cout << "L3 CAT Not Supported\n";

    if ((ebx >> 2) & 1)
        cout << "L2 CAT Supported\n";
    else
        cout << "L2 CAT Not Supported\n";

    if ((ebx >> 3) & 1)
        cout << "MBA Supported\n";
    else
        cout << "MBA Not Supported\n";
}


/**
 * @brief Print the Details of L3 occupancy and bandwidth Monitoring supported
 * 
 */
void static inline checkRDTMonitoring()
{
    uint64_t eax, ebx, ecx, edx;
    eax = 0x0F, ecx = 0; // input EAX= 0FH, ECX= 0
    ebx = 0, edx = 0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax), "c"(ecx)
                 : "memory");
    // cout << "EAX: " << eax << " EBX: " << ebx << " ECX: " << ecx << " EDX: " << edx << "\n";

    // check L3 cache Monitoring, EDX bit1
    if ((edx >> 1) & 1)
        cout << "L3 Cache Monitoring Supported\n";
    else
        cout << "L3 Cache Monitoring Not Supported\n";

    eax = 0x0F, ecx = 1; // Sub-leaf (EAX = 0FH, ECX = 1) 
    ebx = 0, edx = 0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax), "c"(ecx)
                 : "memory");
    // cout << "EAX: " << eax << " EBX: " << ebx << " ECX: " << ecx << " EDX: " << edx << "\n";

    // Return Conversion factor, EBX
    cout<<"Conversion factor from reported IA32_QM_CTR value to occupancy metric:"<<ebx<<"\n";
    
    // Return maxRMID , ECX
    cout<<"Maximum range (zero-based) of RMID of this resource type:0-"<<ecx<<"\n";
    
    // check L3 occupancy Monitoring Support, EDX bit0
    if (edx & 1)
        cout << "L3 occupancy Monitoring Supported\n";
    else
        cout << "L3 occupancy Monitoring Not Supported\n";
    
    // check L3 Total Bandwidth Monitoring, EDX bit1
    if ((edx >> 1) & 1)
        cout << "L3 Total Bandwidth Monitoring Supported\n";
    else
        cout << "L3 Total Bandwidth Monitoring Not Supported\n";
 
    // check L3 Local Bandwidth Monitoring Support, EDX bit2
    if ((edx >> 2) & 1)
        cout << "L3 Local Bandwidth Monitoring Supported\n";
    else
        cout << "L3 Local Bandwidth Monitoring Not Supported\n";


}

int main()
{
    cout << "** Task5: Checking Intel RDT Support **\n";

    if (checkCpuidSupport())
    {
        // Check RDT Capability
        if (checkRDTCapability())
        {
            // check RDT Monitoring support
            checkRDTMonitoring();
            // check RDT Allocation support
            checkRDTAllocation();
        }
    }
    else
        cout << "CPUID Not Supported\n";

    return 0;
}

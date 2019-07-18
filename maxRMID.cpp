/**
 * @file maxRMID.cpp
 * @author Swapnil Raykar (swap612@gmail.com)
 * @brief Prints the maximum number of RMIDs supported
 * @version 0.1
 * @date 2019-07-16
 * 
 * @copyright Copyright (c) 2019 Swapnil Raykar
 * 
 */

#include <iostream>
using namespace std;

#define CPUID_BIT 21

/**
 * @brief Check if CPUID supported by the cpu
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
 * @brief Check for RDT Monitor or Allocation capability 
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
 * @brief Get the Max RMID supported 
 * 
 * @return int 
 */
int static inline getMaxRMID()
{
    uint64_t eax, ebx, ecx, edx;
    eax = 0x0F, ecx = 0; // input EAX= 0x0F, ECX= 0
    ebx = 0, edx = 0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(eax), "c"(ecx)
                 : "memory");
    // cout << "EAX: " << eax << " EBX: " << ebx << " ECX: " << ecx << " EDX: " << edx << "\n";
    return ebx;
}

int main()
{
    cout << "** Task6: Prints the maximum number of RMIDs supported **\n";
    if (checkCpuidSupport())
    {
        if (checkRDTCapability())
            cout << "Range of RMIDs supported: 0 - " << getMaxRMID() << endl;
    }
    else
    {
        cout << "CPUID Not supported";
    }
    return 0;
}

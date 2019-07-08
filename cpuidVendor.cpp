/* Task 4: Write a CPP program to check basic CPUID information (Vendor Name) */

#include <iostream>
#include<cstring>
using namespace std;

#define CPUID_BIT 21

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

void static inline printVendorName()
{
    uint64_t eax, ebx, ecx, edx;
    eax = 0x0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "0"(eax), "2"(ecx)
                 : "memory");
    // cout << eax << ":" << ebx << ":" << ecx << ":" << edx << "\n";
    char vendorName[13];
    memcpy(vendorName, &ebx, 4);
    memcpy(vendorName+4, &edx, 4);
    memcpy(vendorName+8, &ecx, 4);
    vendorName[12]='\0';
    cout<<"Vendor Name: "<<vendorName<<"\n";
}

int main()
{
    cout << "** Task4: Print CPU Vendor Name **\n";

    if (checkCpuidSupport())
    {
        // cout << "CPUID Supported\n";
        printVendorName();
    }
    else
        cout << "CPUID Not Supported\n";

    return 0;
}
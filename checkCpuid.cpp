/* Task 3: Write C/C++ program to check if CPUID supported */

#include<iostream>
using namespace std;

#define CPUID_BIT 21

bool checkCpuidSupport(){
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
                 :"=g"(expFlag), "=g"(modFlag)
                :"r"(setbitmask):"%rax");
    // cout <<expFlag << modFlag ;
    // compare expected and modified values of flag
    if(modFlag == expFlag)     
        return true;
    return false;
}

int main(){
    cout<<"** Task3: Checking CPUID support **\n";
    
    if(checkCpuidSupport())
        cout<<"CPUID Supported\n";
    else
        cout<<"CPUID Not Supported\n";
    
    return 0;    
}
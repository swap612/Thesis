/* Task2: Write a cpp code to read the RFLAGs and check, set and clear any bit */

#include <iostream>
using namespace std;

// check the 'bit' in RFLAG
bool inline static checkFlagBit(int bit){
    uint64_t flags;
    asm volatile("pushfq \n\t "
                 "popq %0"
                 :"=g"(flags));
    cout<<"RFLAGS: "<<flags<<"\t";
    return ( flags & (1 << bit) );
    
}

// Set the 'bit' to 1 in RFLAG
void static inline setFlagBit(int bit){
    uint64_t oldflag=0, newflag,modflag,\
    setbitmask = (1LL << bit); // make 1 at bit position
    cout<<"setbitmask: "<<setbitmask<<"\n";
    asm volatile("pushfq \n\t "
                 "popq %0 \n\t"
                 "pushfq \n\t"
                 "popq %%rax \n\t"
                 "or %3, %%rax \n\t"
                 "mov %%rax, %1 \n\t"
                 "pushq %%rax \n\t"
                 "popfq \n\t"
                 "pushfq \n\t "
                 "popq %2 \n\t"
                 :"=g"(oldflag) , "=g"(newflag), "=g"(modflag)
                :"r"(setbitmask):"%rax");
    cout<<"Old Flags: "<<oldflag<<"\tExpected Flag: "<<newflag<<"\n";
    // cout<<"New Flags: "<< (oldflag | setbitmask)<<"\n";    
    cout<<"Updated flag: "<<modflag;
}

// Clear the 'bit' i.e. Set the 'bit' to 0 in RFLAG
void static inline clearFlagBit(int bit){
    uint64_t oldflag=0, newflag,modflag,\
    setbitmask = ~(1LL << bit); // make 1 at bit position
    cout<<"setbitmask: "<<setbitmask<<"\n";
    asm volatile("pushfq \n\t "
                 "popq %0 \n\t"
                 "pushfq \n\t"
                 "popq %%rax \n\t"
                 "and %3, %%rax \n\t"
                 "mov %%rax, %1 \n\t"
                 "pushq %%rax \n\t"
                 "popfq \n\t"
                 "pushfq \n\t "
                 "popq %2 \n\t"
                 :"=g"(oldflag) , "=g"(newflag), "=g"(modflag)
                :"r"(setbitmask):"%rax");
    cout<<"Old Flags: "<<oldflag<<"\tExpected Flag: "<<newflag<<"\n";
    // cout<<"New Flags: "<< (oldflag | setbitmask)<<"\n";    
    cout<<"Updated flag: "<<modflag;
       
}

/* Main function */
int main(int argc, char **argv)
{

    printf("============================\n * Welcome to Swapnil-lab *\n============================\n");

    //check correct params
    if (argc != 2)
    {
        cout << "Usage:" << argv[0] << " Flag bit(0-64) \n";
        return 0;
    }

    // take bit from cmdline args
    int bit = atoi(argv[1]);

    /* 1. checking if bit is set in flag */
    cout<<"Checking Bit "<<bit<<" in a flag\n" ;
    printf("bit%d: %d\n", bit, checkFlagBit(bit));

    /* 2. Set a particular bit in Flag */
    setFlagBit(bit);
    
    /* 3. Rechecking if setFlagBit works */
    cout<<"\n\nChecking if bit set\n";
    printf("bit%d: %d\n", bit, checkFlagBit(bit));

    /* 4. Clear the set bit */
    cout<<"\nClearing the flag bit\n";
    clearFlagBit(bit);
    
     /* 5. Rechecking if setFlagBit works */
    cout<<"\n\nChecking if bit reset\n";
    printf("bit%d: %d\n", bit, checkFlagBit(bit));

    return 0;
}
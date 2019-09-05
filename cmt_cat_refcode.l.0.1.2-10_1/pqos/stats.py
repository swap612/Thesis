###
# @file stats.py
# @author swapnil Raykar <swap612@gmail.com>
# @brief Calculate the stats i.e. Sum, Count and Average of values present in a file
#
### 


import sys

Tsum=0
count =0
if(len(sys.argv)!=2) :
	print("Invalid number of Arguments\nUsage: {} <InputFile>\nInputFile contains single values on each line ".format(sys.argv[0]))
	sys.exit(0)
with open(sys.argv[1], 'r') as inp:
   for line in inp:
       try:
           num = float(line)
           count+=1
           Tsum += num
       except ValueError:
           print('{} is not a number!'.format(line))
print('LLC occupancy Statistics for {} '.format(sys.argv[1]))
print('Sum: {}\nCount: {} \nAvg: {}'.format(Tsum,count ,Tsum/count))

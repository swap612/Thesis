#!/bin/sh
#####
# @file ins_rmmod.sh
# @author Swapnil Raykar <swap612@gmail.com>
# @brief shell script to insert and remove the module for debugging purpose 
#####

# check for  valid arguments
if [ $# -ne 1 ]
then
        echo  "Invalid arguments!!! \nUsage: $0 <moduleName>"
        exit 1
fi

# get the moduleName from the arg1
moduleName=$1
# direct the clear messages to null, so it doesn't come on terminal 
sudo dmesg -c > /dev/null

# Insert Module
echo "Inserting Module ${moduleName}"
sudo insmod $moduleName.ko
dmesg
sudo dmesg -c > /dev/null

# Remove Module
echo "Removing Module ${moduleName}"
sudo rmmod $moduleName
dmesg
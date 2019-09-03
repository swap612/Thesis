# Script to get the process ID
# Swapnil Raykar

scriptName=$0
# check for number of arguments
if [ $# -ne 1 ] ; then
	echo "Invalid number of arguments"
	echo "Usage: $0 <processName>"
	exit 1 
fi

processName=$1

id=0	# PID get stored in it 
rm -f p.txt p1.txt	# Remove the old files
# search the process with name processName in the process list
ps -aux | grep $processName > p.txt
cat p.txt | grep -v grep > p1.txt
 
# check if no output is return
nlines=$(cat p1.txt | wc -l)
if [ $nlines -eq 0 ] ; then
	echo "No process running with name $processName"	
elif [ $nlines -gt 1 ] ; then 
	printf "Multiple processes with the given name. Use specific process Name\n"
else
	pid=$(awk '{print $2 }' p1.txt)
	echo "Process $processName has pid = $pid" 
fi

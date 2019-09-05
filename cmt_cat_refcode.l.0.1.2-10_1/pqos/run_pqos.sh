###
# @file run_pqos.sh
# @author Swapnil Raykar <swap612@gmail.com>
# @brief Run pqos and calculate the average llc occupancy of two cores
# @args $1: Output file name
#	$2: App1 CoreID
#	$3: App3 CoreID
###

scriptName=$0
# check for number of arguments
if [ $# -ne 3 ] ; then
	echo "Invalid number of arguments"
	echo "Usage: $0 <outFileName> <App1CoreID> <App2CoreID>"
	exit 1 
fi 

PQOS_OUT=$1
L3O_OUT=l3o_$1
SUM_OUT=Sum_$1

CORE1_PATT='<core>'$2'</core>'
CORE2_PATT='<core>'$3'</core>'

CORE1_OUT=Core$2_$1
CORE2_OUT=Core$3_$1

MON_PATT='llc:'+$2,$3

sudo ./pqos -m $MON_PATT -o $PQOS_OUT -u xml -t 10
cat $PQOS_OUT | grep -A 3 $CORE1_PATT | grep l3_occupancy_kB | awk -F ">" '{print $2}' | awk -F "<" '{print $1}' > $CORE1_OUT 
cat $PQOS_OUT | grep -A 3 $CORE2_PATT | grep l3_occupancy_kB | awk -F ">" '{print $2}' | awk -F "<" '{print $1}' > $CORE2_OUT 

python3 stats.py $CORE1_OUT
python3 stats.py $CORE2_OUT

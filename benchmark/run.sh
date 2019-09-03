coreid=$1
make
taskset -c $coreid ./matrix



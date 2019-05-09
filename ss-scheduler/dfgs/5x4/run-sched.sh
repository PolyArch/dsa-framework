#!/bin/bash
mkdir -p output

lat=""
lat_eq=""

lat_eq="0"
time_eq="0"

if [ -z "$1" ]; then
alg="sa"
else
alg=$1
fi

if [ -z "$2" ]; then
subalg="MR.RT"
else
subalg=$2
fi

if [ -z "$3" ]; then
ed=15
else
ed=$3
fi


bench=""

for i in *.dfg; do
  echo "************ $i *************"; 	
  cmd="$SS_TOOLS/bin/sb_sched $SS_TOOLS/configs/softbrain_5x4.sbmodel $i --verbose --algorithm $alg --sub-alg $subalg --show-gams --mipstart --max-edge-delay=$ed --timeout=3600";

  echo $cmd
  $cmd | tee out.txt
  #$cmd > out.txt
  bench="$bench $i"
  lat="$lat `grep "latency:" out.txt | cut -d" " -f 2`"
  time="$time `grep "sched_time:" out.txt | cut -d" " -f 2`"
  lat_eq="$lat_eq+`grep "latency:" out.txt | cut -d" " -f 2`"
  time_eq="$time_eq+`grep "sched_time:" out.txt | cut -d" " -f 2`"
done

echo $bench             | tee sum.txt 
echo $lat               | tee -a sum.txt 
echo $time              | tee -a sum.txt 


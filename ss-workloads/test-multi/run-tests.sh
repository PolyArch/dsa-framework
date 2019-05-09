#!/bin/bash
export SBCONFIG=$SS_TOOLS/configs/revel-1x2.sbmodel


#export LD_LIBRARY_PATH=~/ss-stack/ss_tools/lib


> fail_list 
> pass_list
> lock

function run_test {
  test=$1

  BACKCGRA=1 LINEAR_SCR=1 timeout 10 gem5.opt ~/ss-stack/gem5/configs/example/se.py --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB  --caches  --cmd=$test 
  # BACKCGRA=1 SUPRESS_STATS=1 timeout 10 gem5.opt ~/ss-stack/gem5/configs/example/se.py --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB  --caches  --cmd=$test 
  # BACKCGRA=1 SUPRESS_STATS=1 timeout 10 gem5.opt ~/ss-stack/gem5/configs/example/se.py --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --ruby --num-cpus=16 --num-dirs=16 --network=simple --topology=Mesh_XY --mesh-rows=2 --cmd=$test
  
  ret_val=$?
  { 
  flock $fd 
  echo -n "Test $test Completed, and it "
  if [ "$ret_val" != "0" ]; then
    echo $test FAILED
    fail_list=`cat fail_list`
    echo "FAILED!!!!!!!!!!!!"
    echo "$fail_list $test" > fail_list
  else
    echo "passed"
    pass_list=`cat pass_list`
    echo "$pass_list $test" > pass_list
  fi
  echo
  } {fd}<lock
}


export -f run_test

if [ -z "$1" ]; then
  make -j8

  echo "Test all the cases!"
  #for i in `ls ind*.c | grep -v fix | grep -v unalign`; do
  for i in `ls *.c | grep -v "fix_" | grep -v unalign`; do
    for l in 36 128 2048; do
      test=bin/`basename $i .c`_$l
      sem -j+0 run_test $test
    done
  done

  for i in `ls fix*.c`; do
    test=bin/`basename $i .c`
    sem -j+0 run_test $test
  done

  sem --wait

else
  make $1

  echo "Test " $1
  run_test $1

fi

tests_failed=`wc -w fail_list | cut -d' ' -f1 `
tests_passed=`wc -w pass_list | cut -d' ' -f1 `

tests_total=$((tests_passed + tests_failed))

#if [ "$tests_failed" = "0" ]; then
#  echo All $tests_passed Tests Passed!
#else
  echo $tests_failed / $tests_total Tests FAILED!
  pass_list=`cat pass_list`
  fail_list=`cat fail_list`

  echo "Passing:  $pass_list"
  echo "Failing:  $fail_list"
#fi



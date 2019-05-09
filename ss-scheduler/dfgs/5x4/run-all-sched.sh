#subalg="M.R.T M.RT MR.T MR.RT MR'.RT MRT'.RT  MRT"

#subalg="MR.RT MRT'.RT MR'.RT MR' MRT' MRT"
subalg="MRT' MRT'.RT"
#subalg=MRT
#subalg="MR'.RT MRT'.RT"


logfile=log.txt
sum=summary.txt


for ed in 15 7 3; do

  echo  "ed = $ed" | tee -a $sum
  
  for i in $subalg; do
    echo $i  | tee -a $sum
  
    echo -e "\n\n\n\n\n\n**********          $i            *********" >> $logfile
    run-sched.sh gams $i $ed >> $logfile
  
    cat sum.txt | tee -a $sum
  done

done

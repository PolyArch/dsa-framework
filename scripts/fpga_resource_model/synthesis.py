import subprocess
import os
from os.path import dirname
import glob
import datetime
import time
import random


def synthesize(name, verilog_file, tcl_script, build_folder, output_folder):
    print("Running: " + name + "\n")
    return subprocess.Popen(["vivado -nolog -nojournal -mode batch -source " + tcl_script + 
                " -tclargs --file " + verilog_file + " --name " + name + " --output " + output_folder + " --wkdir " + 
                build_folder], shell=True)

def check_finished():
    global finished_num
    for process in processes:
        process.poll()
        if process.returncode == 0:  # process end
            print("normally end")
            finished_num += 1
            processes.remove(process)
            process.terminate()
            process.kill()
        elif process.returncode != 0 and process.returncode is not None:
            print("abnormally end : kill process")
            finished_num += 1
            processes.remove(process)
            process.terminate()
            process.kill()

print("Starting Parsing Synthesis")

# Get Path to Home Directory
data_folder = "/data"

# Get Path to TCL Script
tcl_script = "./vivado.tcl"
output_folder = data_folder + "/input_vector_port/output"
build_folder = data_folder + "/build"
verilog_folder = data_folder + "/input_vector_port/data"

if not os.path.isdir(output_folder):
    print("Output Folder Doesnt Exist!")
    exit(1)

# Multi-thread parameter
parallel = 64
processes = []
finished_num = 0
has_run_num = 0

start_time = time.time()

verilog_files = glob.glob(verilog_folder + "/**/*.v", recursive=True)

print(len(verilog_files))

# Randomly Make it for files to be used in case of early termination
random.shuffle(verilog_files)

# Get all the Verilog Files in the Verilog Folder
for file in verilog_files:
    # Get File Name
    name = dirname(os.path.realpath(file)).split("/")[-1]
    # Wait for open process
    while(len(processes) >= parallel):
        time.sleep(3)
        intermediate_time = time.time()
        check_finished()
        print("\n" * 75)
        print("######################################################")
        print("Processes Run:", has_run_num, "| Processes Finished:", finished_num, "| Processes Left:", len(verilog_files) - has_run_num)
        print("Elapsed Time:", str(datetime.timedelta(seconds=round(intermediate_time - start_time))), "| Current Processes Running:", len(processes))
        print("######################################################")
        print("\n" * 3)
    else:
        #There is an open process
        process = synthesize(name, file, tcl_script, build_folder, output_folder)
        has_run_num += 1
        processes.append(process)
    print("Finished File:", name)

while(len(processes) > 0):
    time.sleep(3)
    intermediate_time = time.time()
    check_finished()
    print("\n" * 75)
    print("######################################################")
    print("Processes Run:", has_run_num, "| Processes Finished:", finished_num, "| Processes Left:", len(verilog_files) - has_run_num)
    print("Elapsed Time:", str(datetime.timedelta(seconds=round(intermediate_time - start_time))), "| Current Processes Running:", len(processes))
    print("######################################################")
    print("\n" * 3)

end_time = time.time()

print("Finished! Total Time =", str(datetime.timedelta(seconds=round(end_time - start_time))))
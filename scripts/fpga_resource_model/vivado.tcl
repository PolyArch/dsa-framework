# File to Run
set verilog_file "" 
set name ""
set output_directory ""
set working_directory ""

if { $::argc > 0 } {
  for {set i 0} {$i < $::argc} {incr i} {
    set option [string trim [lindex $::argv $i]]
    switch -regexp -- $option {
      "--file" { incr i; set verilog_file [lindex $::argv $i] }
      "--name" { incr i; set name [lindex $::argv $i] }
      "--output" { incr i; set output_directory [lindex $::argv $i] }
      "--wkdir" {incr i; set working_directory [lindex $::argv $i]}
      "--help"         { print_help }
      default {
        if { [regexp {^-} $option] } {
          puts "ERROR: Unknown option '$option' specified, please type '$script_file -tclargs --help' for usage info.\n"
          return 1
        }
      }
    }
  }
}

# Help information for this script
proc print_help {} {
  variable script_file
  puts "\nDescription:"
  puts "Synthesize and Generate Report"
  puts "Syntax:"
  puts "$script_file -tclargs \[--file>\]"
  puts "$script_file -tclargs \[--help\]\n"
  puts "Usage:"
  puts "Name                   Description"
  puts "-------------------------------------------------------------------------\n"
  puts "\[--file <name>\]        Use the specified file.\n"
  puts "\[--name <name>\]        Use the specified project name.\n"
  puts "\[--output <name>\]      Copies reports to specified output folder.\n"
  puts "\[--wkdir <name>\]      Copies reports to specified output folder.\n"
  puts "\[--help\]               Print help information for this script\n"
  puts "-------------------------------------------------------------------------\n"
  exit 0
}

# Create a directory and switch into it
cd $working_directory
file mkdir $name
cd $name

# Creates a work directory
set wrkdir [file join [pwd] obj]

# Set Part
set part_fpga "xcvu9p-flga2104-2L-e"

# Create In-Memory Project File
create_project -part $part_fpga -force $name

# Add Files
set files [list \
 "${verilog_file}" \
]
add_files -norecurse $files

# Synthesize the design
synth_design -top [lindex [find_top] 0] -flatten_hierarchy rebuilt -mode out_of_context

set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
set_property SEVERITY {Warning} [get_drc_checks UCIO-1]

# Create a report directory
set rptdir [file join $wrkdir report]
file mkdir $rptdir

# Report utilization of the current device
set rptutil [file join $rptdir utilization.txt]
report_utilization -hierarchical -file $rptutil

# Report the RAM resources utilized in the implemented design
report_ram_utilization -file $rptutil -append -detail



# change 
exec cp -f -r $rptdir "${output_directory}/${name}"
exec rm -f -r $rptdir "${output_directory}/${name}/report"
exec rm -r $wrkdir

import os
import pandas as pd
from os.path import dirname
import csv

def parse_reports(output_folder, csv_file_path): 
    def parse_utilization(file_name):
        file_lines = [line.rstrip() for line in open(file_name, 'r')]

        data = []
        file_line = file_lines[24]
        file_line = file_line.replace(' ', '')
        return file_line.split('|')[1:-1]

    report_directories = [ f.path for f in os.scandir(output_folder) if f.is_dir() ]


    with open(csv_file_path, 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile, delimiter=',',
                                quotechar='|', quoting=csv.QUOTE_MINIMAL)
        csv_writer.writerow(['File_Name', 'Inputs', 'Outputs', 'Depth', 'Stated', 'Discard', 'BusWidth', 'Padding', 'Module', 'TotalLUTs', 'LogicLUTs', 'LUTRAMs', 'SRLs', 'FFs', 'RAMB36', 'RAMB18', 'URAM', 'DSPBlocks'])
        
        print ("Total Reports:", len(report_directories))
        for report in report_directories:
            report_name = report.split('/')[-1]
            report_params = report_name.split('_')

            for i in range(len(report_params)):
                report_params[i] = report_params[i][:-2]
                
            data = parse_utilization(report + "/utilization.txt")
            combined_params = [report_name] + report_params + data
            csv_writer.writerow(combined_params)

def parse_given_parameters(parameter_file, output_csv):
    data = pd.read_csv(parameter_file)
    data.drop(['mapped.area-Number of ports', 'mapped.area-Number of cells', 
    'mapped.area-Number of combinational cells', 'mapped.area-Number of sequential cells', 
    'mapped.area-Number of macros/black boxes', 'mapped.area-Number of buf/inv', 
    'mapped.area-Number of references', 'mapped.area-Combinational area', 
    'mapped.area-Buf/Inv area', 'mapped.area-Noncombinational area', 
    'mapped.area-Macro/Black Box area', 'mapped.area-Net Interconnect area', 
    'mapped.area-Total cell area', 'mapped.area-Total area', 'mapped.power-Cell Internal Power', 
    'mapped.power-Net Switching Power', 'mapped.power-Total Dynamic Power', 'mapped.power-Total', 
    'mapped.power-Cell Leakage Power', 'mapped.power-io_pad', 'mapped.power-memory', 
    'mapped.power-black_box', 'mapped.power-clock_network', 'mapped.power-register', 
    'mapped.power-sequential', 'mapped.power-combinational', 'mapped.timing-data arrival time', 
    'mapped.timing-data required time', 'mapped.timing-slack', 'designName'], inplace=True, axis=1)
    data.to_csv(output_csv, index=False)

home_dir = dirname(os.path.realpath(__file__))
data_folder = "/data"
output_folder = data_folder + "/output_vector_port/output"
util_csv_file_path = home_dir + "/output_vector_port.csv"

parse_reports(output_folder, util_csv_file_path)

print("parsed", util_csv_file_path)
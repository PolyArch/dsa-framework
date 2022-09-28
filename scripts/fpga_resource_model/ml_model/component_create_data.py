import os
import torch
from torch.utils.data.dataset import Dataset
import numpy as np
import pandas as pd
from os.path import dirname

class DataSet(Dataset):
    def __init__(self, data_list):
        self.data = data_list
        self.data_len = len(data_list)
    
    def __getitem__(self, index):
        single_data = self.data[index]
        return (torch.FloatTensor(single_data[0]), torch.FloatTensor(single_data[1]))

    def __len__(self):
        return self.data_len


def create_network_data():
    home_dir = dirname(os.path.realpath(__file__))
    parameter_csv_file_path = home_dir + "/component.csv"
    data = pd.read_csv(parameter_csv_file_path)
    data.dropna()


    # Drop values we don't care about
    data.drop(['File_Name', 'Instance', 
                'Module', 'SRLs', 'RAMB36', 'RAMB18', 'URAM', 'DSPBlocks'], inplace=True, axis=1)
    columns_titles = ['Inputs', 'Outputs', 'Depth', 'Static', 'Granularity', 'Datawidth', 'TotalLUTs', 'LogicLUTs', 'LUTRAMs', 'FFs']
    data=data.reindex(columns=columns_titles)

    # For each Datapoint, split into [input, output]
    # Convert to List
    data_list = data.values.tolist()

    print(data.columns, 'columns')
    print(len(data_list), 'data points')

    # For each Datapoint, split into [input, output]
    network_data = []
    for data in data_list:
        network_data.append((data[:-4], data[-4:]))

    return DataSet(network_data)

if __name__ == "__main__":
    create_network_data()
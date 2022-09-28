from functools import partial
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import torch.nn.functional as F
from torch.utils.data import random_split
from component_create_data import create_network_data
from torchvision import transforms
from torch.utils.data.dataset import Dataset
from torch.utils.data.sampler import SubsetRandomSampler
from ray import tune
from ray.tune import CLIReporter
from ray.tune.schedulers import ASHAScheduler
from ray.tune.suggest.skopt import SkOptSearch
import os
import sys

import numpy as np


'''
Input: [inputs, outputs, depth, static]
Output: [TotalLUTs, LogicLUTs, LUTRAMs, FFs]
'''
class NeuralNetwork(nn.Module):
    def __init__(self, in_features, out_features, hidden_features=50):
        super(NeuralNetwork, self).__init__()
        self.fc1 = nn.Linear(in_features, hidden_features)
        self.fc2 = nn.Linear(hidden_features, hidden_features)
        self.fc3 = nn.Linear(hidden_features, out_features)

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        return x

'''
Calculates the RMSE for data. Used to test.
'''
class RMSELoss(nn.Module):
    def __init__(self, eps=1e-6):
        super().__init__()
        self.mse = nn.MSELoss()
        self.eps = eps
        
    def forward(self,yhat,y):
        loss = torch.sqrt(self.mse(yhat,y) + self.eps)
        return loss


'''
Tests the network. Returns the average loss per example
'''
def TestNework(model, criterion, test_loader):
    examples = 0
    total_losses = torch.Tensor([0, 0, 0, 0])
    for i, data in enumerate(test_loader, 0):
        with torch.no_grad():
            inputs, labels = data
            outputs = model(inputs)
        
            for output, given_output in zip(labels, outputs):
                total_losses += criterion(output, given_output)
    return total_losses / len(test_loader)

def AverageTrainingHardware(model, test_loader):
    examples = 0
    model_total = torch.Tensor([0, 0, 0, 0])
    real_total = torch.Tensor([0, 0, 0, 0])
    for i, data in enumerate(test_loader, 0):
        with torch.no_grad():
            inputs, labels = data
            outputs = model(inputs)
            for output, given_output in zip(labels, outputs):
                real_total += output
                model_total += given_output
                examples += 1
        
    return model_total / examples, real_total / examples

def load_data():
    data = create_network_data()

    test_abs = int(len(data) * 0.2)
    train_abs = int((len(data) - test_abs) * 0.8)
    validation_abs = int((len(data) - (test_abs + train_abs)))
    
    test_set, train_set, validation_set = random_split(
        data, [test_abs, train_abs, validation_abs])

    return test_set, train_set, validation_set

def train_model(config, checkpoint_dir=None):
    model = NeuralNetwork(6, 4)

    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=config["lr"])

    if checkpoint_dir:
        model_state, optimizer_state = torch.load(
            os.path.join(checkpoint_dir, "checkpoint"))
        net.load_state_dict(model_state)
        optimizer.load_state_dict(optimizer_state)

    test_set, train_set, validation_set = load_data()

    trainloader = torch.utils.data.DataLoader(
        train_set,
        batch_size=int(config["batch_size"]),
        shuffle=True,
        num_workers=8)
    valloader = torch.utils.data.DataLoader(
        validation_set,
        batch_size=int(config["batch_size"]),
        shuffle=True,
        num_workers=8)

    for epoch in range(int(config["epochs"])):  # loop over the dataset multiple times
        running_loss = 0.0
        epoch_steps = 0
        for i, data in enumerate(trainloader, 0):
            # get the inputs; data is a list of [inputs, labels]
            inputs, labels = data

            # zero the parameter gradients
            optimizer.zero_grad()

            # forward + backward + optimize
            outputs = model(inputs)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()

            # print statistics
            running_loss += loss.item()
            epoch_steps += 1
            if i % 2000 == 1999:  # print every 2000 mini-batches
                print("[%d, %5d] loss: %.3f" % (epoch + 1, i + 1,
                                                running_loss / epoch_steps))
                running_loss = 0.0

        # Validation loss
        val_loss = 0.0
        val_steps = 0
        total = 0
        total_losses = torch.Tensor([0, 0, 0, 0])
        r1_total_losses = torch.Tensor([0, 0, 0, 0])

        for i, data in enumerate(valloader, 0):
            with torch.no_grad():
                inputs, labels = data
                outputs = model(inputs)
            
                for output, given_output in zip(labels, outputs):
                    total_losses += criterion(output, given_output)
                    r1_total_losses += nn.L1Loss()(output, given_output)
                    total += 1

                loss = criterion(outputs, labels)
                val_loss += loss.cpu().numpy()
                val_steps += 1
        
        total_losses = total_losses / total
        r1_total_losses = r1_total_losses / total

        with tune.checkpoint_dir(epoch) as checkpoint_dir:
            path = os.path.join(checkpoint_dir, "checkpoint")
            torch.save((model.state_dict(), optimizer.state_dict()), path)
        

        tune.report(loss=(val_loss / val_steps), l1_loss=r1_total_losses)
    print("Finished Training")

def main():
    config = {
        "lr": tune.loguniform(1e-4, 1e-1),
        "batch_size": tune.choice([2, 4, 8, 16]),
        "epochs": tune.choice(range(50, 200, 10))
    }
    
    scheduler = ASHAScheduler(
        metric="loss",
        mode="min",
        max_t=200,
        grace_period=1,
        reduction_factor=2)

    skopt_search = SkOptSearch(
        metric="loss",
        mode="min")


    reporter = CLIReporter(
        # parameter_columns=["l1", "l2", "lr", "batch_size"],
        metric_columns=["loss", "l1_loss", "training_iteration"])
    
    result = tune.run(
        partial(train_model),
        resources_per_trial={"cpu": 5},
        config=config,
        num_samples=10,
        scheduler=scheduler,
        search_alg=skopt_search,
        progress_reporter=reporter)

    test_set, train_set, validation_set = load_data()

    test_loader = torch.utils.data.DataLoader(
        test_set,
        batch_size=1,
        shuffle=True,
        num_workers=8)

    best_trial = result.get_best_trial("loss", "min", "last")
    print("Best trial config: {}".format(best_trial.config))
    print("Best trial final validation loss: {}".format(
        best_trial.last_result["loss"]))
    print("Best trial l1 loss: {}".format(best_trial.last_result["l1_loss"]))

    best_trained_model = NeuralNetwork(6, 4)

    best_checkpoint_dir = best_trial.checkpoint.value
    model_state, optimizer_state = torch.load(os.path.join(
        best_checkpoint_dir, "checkpoint"))
    best_trained_model.load_state_dict(model_state)
    

    l1Loss = TestNework(best_trained_model, nn.L1Loss(), test_loader).numpy()
    mseLoss = TestNework(best_trained_model, nn.MSELoss(), test_loader).numpy()
    rmseLoss = TestNework(best_trained_model, RMSELoss(), test_loader).numpy()

    averages = AverageTrainingHardware(best_trained_model, test_loader)

    model_average = averages[0].numpy()
    real_average = averages[1].numpy()

    print("Average Loss with L1      :", l1Loss)
    print("Average Loss with MSE loss:", mseLoss)
    print("Average Loss with RMS Loss:", rmseLoss)
    print("Average Model:", model_average)
    print("Real Model:", real_average)

    inputs, outputs = test_set[0]

    traced_script_module = torch.jit.trace(best_trained_model, inputs)
    traced_script_module.save("models/component_model.pt")

    lines = ['Component Model, trained on ' + str(len(train_set)) + ' samples',
            'Trained using Pytorch, RayTune, and SciOptimize',
            '',
            'Parameters',
            '   Learning Rate: ' + str(best_trial.config["lr"]),
            '   Batch Size: ' + str(best_trial.config["batch_size"]),
            '   Epochs: ' + str(best_trial.config["epochs"]),
            'Best trial final validation loss: ' + str(best_trial.last_result["loss"]),
            'Best trial l1 loss: ' + str(best_trial.last_result["l1_loss"]),
            '',
            'Average Loss with L1 ' + str(l1Loss), 
            'Average Loss with MSE loss: ' + str(mseLoss),
            'Average Loss with RMS Loss: ' + str(rmseLoss),
            '',
            'Average Model: ' + str(model_average),
            'Real: ' + str(real_average)
            ]
    with open('models/component_model_info.txt', 'w') as f:
        f.write('\n'.join(lines))


if __name__ == "__main__":
    main()
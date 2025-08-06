#!/bin/bash

# Setup frequency
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
echo 2600000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
echo 2600000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq

# Setup RDMA NIC
sudo ip addr add 18.18.1.1/24 dev ibs785f0
sudo ip link set ibs785f0 up
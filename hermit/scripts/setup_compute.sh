#!/bin/bash

# Setup frequency
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
echo 2600000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
echo 2600000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq

# Setup RDMA NIC
sudo ip addr add 18.18.1.2/24 dev ibs785f0
sudo ip link set ibs785f0 up

# Setup Hermit
echo Y | sudo tee /sys/kernel/debug/hermit/apt_reclaim
echo Y | sudo tee /sys/kernel/debug/hermit/batch_io
echo Y | sudo tee /sys/kernel/debug/hermit/batch_tlb
echo Y | sudo tee /sys/kernel/debug/hermit/batch_account
echo Y | sudo tee /sys/kernel/debug/hermit/bypass_swapcache
echo Y | sudo tee /sys/kernel/debug/hermit/speculative_io
echo Y | sudo tee /sys/kernel/debug/hermit/speculative_lock
echo 4 | sudo tee /sys/kernel/debug/hermit/sthd_cnt
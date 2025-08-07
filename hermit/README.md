# Configuration
For hermit, we use rs3labsrv5 as the compute node, rs3labsrv4 as the server
node.

# Setup
1. On the compute node, execute `./scripts/setup_compute.sh`
2. On the memory node, start the memory server
```bash
# Compile the server
cd mage-artifact-baseline/hermit/remoteswap/server
make
# Setup the network
./setup_memory.sh
# Start memory server
./rswap-server 18.18.1.1 9400 96 56
```
3. On the compute node
```bash
cd mage-artifact-baseline/hermit/remoteswap/client
make
sudo ./manage_rswap_client.sh install
```

# Experiments
## Figure 9(a)
On the compute node, run
```
cd mage-artifact-baseline/hermit
./scipts/bench-gapbs-hermit.sh
```
This script will compile gapbs, run it and generate one column starting with `hermit 48` in `~/benchmark-out-ae/gapbs/gapbs.txt`

## Figure 9(b)
On the compute node, run
```
cd mage-artifact-baseline/hermit
./scipts/bench-xsbench-hermit.sh
```
This script will compile xsbench, run it and generate one column starting with `hermit 48` in `~/benchmark-out-ae/xsbench/xsbench.txt`

## Figure 10
On the compute node, run
```
cd mage-artifact-baseline/hermit
./scipts/bench-seq-scan-hermit.sh
```


## Figure 11


## Figure 12(a)

## Figure 12(b)

## Figure 13(a)

## Figure 13(b)

## Figure 14

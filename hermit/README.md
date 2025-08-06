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
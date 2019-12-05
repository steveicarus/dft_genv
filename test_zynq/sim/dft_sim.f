#
# This is build instrucftions for the dft_sim.out simulation. The top
# level DFT_SIM.v module instantiates the FPGA design in the ver/
# directory. It is enough to reference the directory and a library,
# as the modules will be picked up automatically.
#
# The simulation root uses SIMBUS to provide an AXI4 bus, which we
# connect to the DFT_FPGA module. That's all we need to do with the
# verilog source.

# This is the simulation root module.
DFT_SIM.v

# This is to simulate Xilinx loading.
#$(XILINX_VER)/glbl.v

# The device-under-test lives here
+libdir+../ver

# SIMBUS support files are here
+libdir+$(SIMBUS_VER)

# Xilinx libraries
#+libdir+$(XILINX_VER)/unisims

/*
 * Copyright (c) 2019 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

`default_nettype none
`timescale 1ps/1ps

/*
 * This is a thin wrapper that is compatible with the Xilinx Vivado
 * design editor. This is necessary because Vivado requires that the
 * top modules that are placed on the design diagram may not be
 * SystemVerilog, and DFT_FPGA is indeed SystemVerilog. It is OK for
 * modules instantiated by the top module to be SystemVerilog. So
 * all this wrapper does is provide syntax sugar to make Vivado
 * happy.
 */

module DFT_FPGA_WRAP
  #(parameter ADDR_WIDTH = 24,
    parameter BURST_LEN_ORDER = 4,
    parameter DFT_SAMPLES = 32,
    parameter DFT_WIDTH   = 24,
    parameter DFT_FRAC    =  8
    /* */)
   (// Global signals
    input wire 			AXI_S_ACLK,
    input wire 			AXI_S_ARESETn,
    // Write address channel
    input wire 			AXI_S_AWVALID,
    output wire 		AXI_S_AWREADY,
    input wire [ADDR_WIDTH-1:0] AXI_S_AWADDR,
    input wire [2:0] 		AXI_S_AWPROT,
    // Write data channel
    input wire 			AXI_S_WVALID,
    output wire 		AXI_S_WREADY,
    input wire [31:0] 		AXI_S_WDATA,
    input wire [3:0] 		AXI_S_WSTRB,
    // Write response channel
    output wire 		AXI_S_BVALID,
    input wire 			AXI_S_BREADY,
    output wire [1:0] 		AXI_S_BRESP,
    // Read address channel
    input wire 			AXI_S_ARVALID,
    output wire 		AXI_S_ARREADY,
    input wire [ADDR_WIDTH-1:0] AXI_S_ARADDR,
    input wire [2:0] 		AXI_S_ARPROT,
    // Read data channel
    output wire 		AXI_S_RVALID,
    input wire 			AXI_S_RREADY,
    output wire [31:0] 		AXI_S_RDATA,
    output wire [1:0] 		AXI_S_RRESP,
    // interrupts
    output wire 		INTERRUPT
    /*  */);


   DFT_FPGA #(.ADDR_WIDTH(ADDR_WIDTH),
	      .BURST_LEN_ORDER(BURST_LEN_ORDER),
	      .DFT_SAMPLES(DFT_SAMPLES),
	      .DFT_WIDTH(DFT_WIDTH),
	      .DFT_FRAC(DFT_FRAC)) dft_fpga_wrapped
     (.AXI_S_ACLK(AXI_S_ACLK),
      .AXI_S_ARESETn(AXI_S_ARESETn),

      .AXI_S_AWVALID(AXI_S_AWVALID),
      .AXI_S_AWREADY(AXI_S_AWREADY),
      .AXI_S_AWADDR(AXI_S_AWADDR),
      .AXI_S_AWPROT(AXI_S_AWPROT),

      .AXI_S_WVALID(AXI_S_WVALID),
      .AXI_S_WREADY(AXI_S_WREADY),
      .AXI_S_WDATA(AXI_S_WDATA),
      .AXI_S_WSTRB(AXI_S_WSTRB),

      .AXI_S_BVALID(AXI_S_BVALID),
      .AXI_S_BREADY(AXI_S_BREADY),
      .AXI_S_BRESP(AXI_S_BRESP),

      .AXI_S_ARVALID(AXI_S_ARVALID),
      .AXI_S_ARREADY(AXI_S_ARREADY),
      .AXI_S_ARADDR(AXI_S_ARADDR),
      .AXI_S_ARPROT(AXI_S_ARPROT),

      .AXI_S_RVALID(AXI_S_RVALID),
      .AXI_S_RREADY(AXI_S_RREADY),
      .AXI_S_RDATA(AXI_S_RDATA),
      .AXI_S_RRESP(AXI_S_RRESP),

      .INTERRUPT(INTERRUPT)
      /* */);
   
endmodule // DFT_FPGA_WRAP

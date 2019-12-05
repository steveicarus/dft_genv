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
 * CLOCKS -
 * This simulation uses the ACLK from the axi4_slave_slot as the master
 * clock for everything. In the real FPGA, we would hook that clock up
 * to one of the Zynq clock sources.
 *
 * AXI4 GP
 * The FPGA has an AXI4 slave port that is connected to the PS GP port
 * as a slave. In this simulation, it is connected to the axi4_slave_slot
 * so that the C coded simulation can drive it.
 */
module DFT_SIM;

   localparam REGS_ADDR_WIDTH = 24;

   initial $dumpvars;

   wire global_aclk;
   wire global_areset_n;
   // Write address channel
   wire regs_awvalid;
   wire regs_awready;
   wire [REGS_ADDR_WIDTH-1:0] regs_awaddr;
   wire [2:0] 		      regs_awprot;
   // Write data channel
   wire 		      regs_wvalid;
   wire 		      regs_wready;
   wire [31:0] 		      regs_wdata;
   wire [3:0] 		      regs_wstrb;
   // Write response channel
   wire 		      regs_bvalid;
   wire 		      regs_bready;
   wire [1:0] 		      regs_bresp;
   // Read address channel
   wire 		      regs_arvalid;
   wire 		      regs_arready;
   wire [REGS_ADDR_WIDTH-1:0] regs_araddr;
   wire [2:0] 		      regs_arprot;
   // Read data channel
   wire 		      regs_rvalid;
   wire 		      regs_rready;
   wire [31:0] 		      regs_rdata;
   wire [1:0] 		      regs_rresp;

   // Interrupts
   wire 		      regs_irq;

   axi4_slave_slot #(.name("DFT_REGS"), .data_width(32), .addr_width(REGS_ADDR_WIDTH), .irq_width(1)) regs_slot
     (// Global signals
      .ACLK   (global_aclk),
      .ARESETn(global_areset_n),
      // Write address channel
      .AWVALID(regs_awvalid),
      .AWREADY(regs_awready),
      .AWADDR (regs_awaddr),
      .AWPROT (regs_awprot),
      // Write data channel
      .WVALID (regs_wvalid),
      .WREADY (regs_wready),
      .WDATA  (regs_wdata),
      .WSTRB  (regs_wstrb),
      // Write response channel
      .BVALID (regs_bvalid),
      .BREADY (regs_bready),
      .BRESP  (regs_bresp),
      .BID    (4'd0),
      // Read address channel
      .ARVALID(regs_arvalid),
      .ARREADY(regs_arready),
      .ARADDR (regs_araddr),
      .ARPROT (regs_arprot),
      // Read data channel
      .RVALID (regs_rvalid),
      .RREADY (regs_rready),
      .RDATA  (regs_rdata),
      .RRESP  (regs_rresp),
      .RID    (4'd0),

      .IRQ    (regs_irq)
      /* */);

   DFT_FPGA #(.ADDR_WIDTH(REGS_ADDR_WIDTH)) dft_fpga
     (// Global signals
      .AXI_S_ACLK   (global_aclk),
      .AXI_S_ARESETn(global_areset_n),
      // Write address channel
      .AXI_S_AWVALID(regs_awvalid),
      .AXI_S_AWREADY(regs_awready),
      .AXI_S_AWADDR (regs_awaddr),
      .AXI_S_AWPROT (regs_awprot),
      // Write data channel
      .AXI_S_WVALID (regs_wvalid),
      .AXI_S_WREADY (regs_wready),
      .AXI_S_WDATA  (regs_wdata),
      .AXI_S_WSTRB  (regs_wstrb),
      // Write response channel
      .AXI_S_BVALID (regs_bvalid),
      .AXI_S_BREADY (regs_bready),
      .AXI_S_BRESP  (regs_bresp),
      // Read address channel
      .AXI_S_ARVALID(regs_arvalid),
      .AXI_S_ARREADY(regs_arready),
      .AXI_S_ARADDR (regs_araddr),
      .AXI_S_ARPROT (regs_arprot),
      // Read data channel
      .AXI_S_RVALID (regs_rvalid),
      .AXI_S_RREADY (regs_rready),
      .AXI_S_RDATA  (regs_rdata),
      .AXI_S_RRESP  (regs_rresp),
      // Interrupt
      .INTERRUPT    (regs_irq)
      /* */);

endmodule // DFT_SIM

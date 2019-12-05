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

/*
 * 0x000000    [31: 0]    CommandStatus
 *                 [0] rw - dft reset
 *                 [1] ro - dft ready
 * 0x000004     [31:0] rw DFT_IDX
 * 0x000008    [31: 0] ro DFT_Config
 *              [7: 0]    - DFT_FRAC
 *             [15: 8]    - DFT_WIDTH
 *             [31:16]    - DFT_SAMPLES
 * 0x00000c
 * 0x000010    [31: 0] ro DFT_REAL
 * 0x000014    [31: 0] ro DFT_IMAG
 * 0x000100    [31: 0] ro BuildId
 * 0x000104 - 0x0001ff ro VersionText
 * 
 * 0x800000     [23:0] rw SRC_REAL[n]
 * 0xc00000     [23:0] rw SRC_IMAG[n]
 */
`default_nettype none
`timescale 1ps/1ps

module DFT_FPGA
  #(parameter ADDR_WIDTH = 24,
    parameter BURST_LEN_ORDER = 4,
    parameter DFT_SAMPLES = 32,
    parameter DFT_WIDTH   = 24,
    parameter DFT_FRAC    =  8
    /* */)
   (// AXI4 port connected to a GP port. This is a slave port, with
    // the processor the master.
    
    // Global signals
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

   localparam [ADDR_WIDTH-1:0] ADDRESS_CMD_STAT  = 'h00_0000;
   localparam [ADDR_WIDTH-1:0] ADDRESS_DFT_IDX   = 'h00_0004;
   localparam [ADDR_WIDTH-1:0] ADDRESS_DFT_Config= 'h00_0008;
   localparam [ADDR_WIDTH-1:0] ADDRESS_DFT_REAL  = 'h00_0010;
   localparam [ADDR_WIDTH-1:0] ADDRESS_DFT_IMAG  = 'h00_0014;
   localparam [ADDR_WIDTH-1:0] ADDRESS_BUILD_ID  = 'h00_0100;
   localparam [ADDR_WIDTH-1:0] ADDRESS_SRC_REAL  = 'h80_0000;
   localparam [ADDR_WIDTH-1:0] ADDRESS_SRC_IMAG  = 'hc0_0000;

   // Make an active-high version of the reset signal. The reset goes
   // to so much stuff, that we want it buffered. We might as well make
   // it active high at the same time.
   reg 				reset_int;
   always @(posedge AXI_S_ACLK) reset_int <= ~AXI_S_ARESETn;

   // Receive the write address.
   reg [ADDR_WIDTH-1:0] write_address;
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	write_address <= 0;
     end else if (AXI_S_AWREADY & AXI_S_AWVALID) begin
	write_address <= AXI_S_AWADDR;
     end

   // Receive the read address
   reg [ADDR_WIDTH-1:0] read_address;
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	read_address <= 0;
     end else if (AXI_S_ARREADY & AXI_S_ARVALID) begin
	read_address <= AXI_S_ARADDR;
     end

   // The build id of the device. This module, and the output patterns
   // it emits, is generated at compile time.
   wire [31:0] build_id;
   BUILD build_id_mod(.build_id(build_id));

   reg 	       dft_reset;
   wire        dft_ready;
   wire [31:0] cmd_stat = {30'h0, dft_ready, dft_reset};
   wire        DFT_CmdStat_hit_w = (write_address == ADDRESS_CMD_STAT);

   localparam IDX_WIDTH = $clog2(DFT_SAMPLES);
   reg [IDX_WIDTH-1:0]  dft_idx;
   wire 		DFT_IDX_hit_w = write_address == ADDRESS_DFT_IDX;
   wire [DFT_WIDTH-1:0] dft_real, dft_imag;

   reg [DFT_SAMPLES-1:0][DFT_WIDTH-1:0] src_real, src_imag;
   wire 			    DFT_SRC_REAL_hit_w = ((write_address&24'hc0_0000) == ADDRESS_SRC_REAL);
   wire 			    DFT_SRC_IMAG_hit_w = ((write_address&24'hc0_0000) == ADDRESS_SRC_IMAG);


   // This state machine drives the read process. Wait for a read
   // address. When we get the read address, mark the read as ready,
   // holding off the next address. When the read completes, then
   // be ready for the next address. To wit:
   //
   //    ARREADY: + + + - - - +
   //    ARVALID: - - + _ . . .
   //    RREADY : . . . . - + -
   //    RVALID : - - - - + + -
   //
   reg 	      reg_s_arready;
   reg 	      reg_s_rvalid;
   reg [31:0] reg_s_rdata;
   assign     AXI_S_ARREADY = reg_s_arready;
   assign     AXI_S_RVALID  = reg_s_rvalid;
   assign     AXI_S_RDATA   = reg_s_rdata;
   assign     AXI_S_RRESP   = 2'b00;

   // Deliver the addressed read register contents to the read data
   // buffer, ready for a read cycle. It is safe and efficient to just
   // track the read address, because the state machine will assure
   // that only one read is in progress, and will only flag the data
   // as valid if it really is valid.
   always @(posedge AXI_S_ACLK)
     case (read_address)
       ADDRESS_CMD_STAT  : reg_s_rdata <= cmd_stat;
       ADDRESS_BUILD_ID  : reg_s_rdata <= build_id;
       ADDRESS_DFT_Config: reg_s_rdata <= {DFT_SAMPLES[15:0], DFT_WIDTH[7:0], DFT_FRAC[7:0]};
       ADDRESS_DFT_REAL  : reg_s_rdata <= dft_real;
       ADDRESS_DFT_IMAG  : reg_s_rdata <= dft_imag;
       default           : reg_s_rdata <= 32'd0;
     endcase

   reg 	      read_address_ready_del;

   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	reg_s_arready <= 1'b1;
	reg_s_rvalid  <= 1'b0;
	read_address_ready_del <= 1'b0;

     end else if (AXI_S_ARREADY && AXI_S_ARVALID) begin
	// If a read address arrives, then switch to the read step. Put
	// in a delay to give a chance for the read address to collect
	// the read data.
	reg_s_arready <= 1'b0;
	read_address_ready_del <= 1'b1;

     end else if (read_address_ready_del) begin
	// The read address was delivered, and the correct data was clocked
	// into the read_data register. Now set it up to be read by the
	// processor.
	read_address_ready_del <= 1'b0;
	reg_s_rvalid <= 1'b1;

     end else if (AXI_S_RREADY && AXI_S_RVALID) begin
	// The processor accepts the read response, so now we can go back
	// to waiting for the next read address.
	reg_s_rvalid  <= 1'b0;
	reg_s_arready <= 1'b1;

     end else begin
	// Waiting for something to happen.
     end

   // The write cycle involves 3 channels: the write address, the
   // write data, and the write response. The state machine flows
   // the state by waiting for the address channel, then the write
   // data channel, and then sending a response.
   reg 			       reg_s_awready;
   reg 			       reg_s_wready;
   reg 			       reg_s_bvalid;
   assign AXI_S_AWREADY = reg_s_awready;
   assign AXI_S_WREADY  = reg_s_wready;
   assign AXI_S_BVALID  = reg_s_bvalid;
   assign AXI_S_BRESP   = 2'b00;
   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	reg_s_awready <= 1'b1;
	reg_s_wready  <= 1'b0;
	reg_s_bvalid  <= 1'b0;

     end else if (AXI_S_AWREADY & AXI_S_AWVALID) begin
	// An address has been sent. Get ready to receive data.
	reg_s_awready <= 1'b0;
	reg_s_wready  <= 1'b1;

     end else if (AXI_S_WREADY & AXI_S_WVALID) begin
	// Data has been sent. Now send the response.
	reg_s_wready  <= 1'b0;
	reg_s_bvalid  <= 1'b1;

     end else if (AXI_S_BREADY & AXI_S_BVALID) begin
	// Sender has received the response. Go back to
	// waiting for an address.
	reg_s_awready <= 1'b1;
	reg_s_wready  <= 1'b0;
	reg_s_bvalid  <= 1'b0;

     end else begin
     end

   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	dft_reset <= 1'b1;
     end else if (DFT_CmdStat_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	dft_reset <= AXI_S_WDATA[0];
     end

   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
	dft_idx <= 0;
     end else if (DFT_IDX_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	dft_idx <= AXI_S_WDATA[IDX_WIDTH-1:0];
     end

   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
     end else if (DFT_SRC_REAL_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	src_real[write_address[21:2]] <= AXI_S_WDATA;
     end

   always @(posedge AXI_S_ACLK)
     if (reset_int) begin
     end else if (DFT_SRC_IMAG_hit_w & AXI_S_WREADY & AXI_S_WVALID) begin
	src_imag[write_address[21:2]] <= AXI_S_WDATA;
     end

   // Combine all the interrupt sources, to generate a single
   // interrupt output.
   assign INTERRUPT = 0;

   idft_slice #(.WIDTH(DFT_WIDTH), .FRAC(DFT_FRAC)) slice
     (.clk(AXI_S_ACLK),
      .reset(dft_reset),
      .ready(dft_ready),
      .dft_real(dft_real),
      .dft_imag(dft_imag),
      .dft_idx(dft_idx),
      .src_real(src_real),
      .src_imag(src_imag)
      /* */);

endmodule // SLF_FPGA

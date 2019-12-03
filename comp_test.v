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

`define N4
module main;

   parameter WIDTH = 24;
   parameter FRAC = 8;

`ifdef N32
   parameter SAMPLES = 32;
   // This is the test data set.
   reg [31:0][WIDTH-1:0] src_real, src_imag;
   initial begin
      src_real[ 0] =  (1<<FRAC);  src_imag[ 0] = 0;
      src_real[ 1] =  (1<<FRAC);  src_imag[ 1] = 0;
      src_real[ 2] =  (1<<FRAC);  src_imag[ 2] = 0;
      src_real[ 3] =  (1<<FRAC);  src_imag[ 3] = 0;
      src_real[ 4] =  (1<<FRAC);  src_imag[ 4] = 0;
      src_real[ 5] =  (1<<FRAC);  src_imag[ 5] = 0;
      src_real[ 6] =  (1<<FRAC);  src_imag[ 6] = 0;
      src_real[ 7] =  (1<<FRAC);  src_imag[ 7] = 0;
      src_real[ 8] = -(1<<FRAC);  src_imag[ 8] = 0;
      src_real[ 9] = -(1<<FRAC);  src_imag[ 9] = 0;
      src_real[10] = -(1<<FRAC);  src_imag[10] = 0;
      src_real[11] = -(1<<FRAC);  src_imag[11] = 0;
      src_real[12] = -(1<<FRAC);  src_imag[12] = 0;
      src_real[13] = -(1<<FRAC);  src_imag[13] = 0;
      src_real[14] = -(1<<FRAC);  src_imag[14] = 0;
      src_real[15] = -(1<<FRAC);  src_imag[15] = 0;
      src_real[16] =  (1<<FRAC);  src_imag[16] = 0;
      src_real[17] =  (1<<FRAC);  src_imag[17] = 0;
      src_real[18] =  (1<<FRAC);  src_imag[18] = 0;
      src_real[19] =  (1<<FRAC);  src_imag[19] = 0;
      src_real[20] =  (1<<FRAC);  src_imag[20] = 0;
      src_real[21] =  (1<<FRAC);  src_imag[21] = 0;
      src_real[22] =  (1<<FRAC);  src_imag[22] = 0;
      src_real[23] =  (1<<FRAC);  src_imag[23] = 0;
      src_real[24] = -(1<<FRAC);  src_imag[24] = 0;
      src_real[25] = -(1<<FRAC);  src_imag[25] = 0;
      src_real[26] = -(1<<FRAC);  src_imag[26] = 0;
      src_real[27] = -(1<<FRAC);  src_imag[27] = 0;
      src_real[28] = -(1<<FRAC);  src_imag[28] = 0;
      src_real[29] = -(1<<FRAC);  src_imag[29] = 0;
      src_real[30] = -(1<<FRAC);  src_imag[30] = 0;
      src_real[31] = -(1<<FRAC);  src_imag[31] = 0;
   end // initial begin
`endif //  `ifdef N32
`ifdef N4
   // Expected output
   //(    5.00000     0.00000) (00000500 00000000)
   //(    0.00781    -0.99609) (00000002 ffffff01)
   //(    3.00391     0.00000) (00000301 00000000)
   //(    0.00781     1.00000) (00000002 00000100)

   parameter SAMPLES = 4;
   reg[3:0][WIDTH-1:0] src_real, src_imag;
   initial begin
      src_real[ 0] =  (2<<FRAC);  src_imag[ 0] = 0;
      src_real[ 1] =  (1<<FRAC);  src_imag[ 1] = 0;
      src_real[ 2] =  (2<<FRAC);  src_imag[ 2] = 0;
      src_real[ 3] =  (0<<FRAC);  src_imag[ 3] = 0;
   end
`endif

   reg  clk;
   reg  reset;
   wire ready;

   // The output component.
   wire [WIDTH-1:0] dft_real, dft_imag;
   // Select the component to calculate
   reg [$clog2(SAMPLES):0] dft_idx;

   // This is the device under test.
   idft_comp_N4 #(.WIDTH(WIDTH), .FRAC(FRAC)) dut
     (.clk(clk),
      .reset(reset),
      .ready(ready),
      // Output component.
      .dft_real(dft_real),
      .dft_imag(dft_imag),
      // Calculate this component.
      .dft_idx (dft_idx[$clog2(SAMPLES)-1:0]),
      // Input vector
      .src_real(src_real),
      .src_imag(src_imag)
      /* */);

   initial begin
      $dumpvars(0, main);
      clk = 1;
      reset = 1;
      #1 clk = 0;
      #1 clk = 1;
      #1 clk = 0;
      reset = 0;
      #1 clk = 1;

      for (dft_idx = 0 ; dft_idx < SAMPLES; dft_idx = dft_idx+1) begin
	 reset <= 0;
	 #1 clk = 0;
	 #1 clk = 1;
	 while (ready != 1'b1) begin
	    #1 clk = 0;
	    #1 clk = 1;
	 end
	 $display("(%h %h)", dft_real, dft_imag);
	 reset <= 1;
	 #1 clk = 0;
	 #1 clk = 1;
      end
      #1 clk = 0;
      #1 clk = 1;
      #1 $finish;
   end
endmodule // main

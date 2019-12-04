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

module main;

   parameter SAMPLES = `USE_SAMPLES;
   parameter WIDTH = 24;
   parameter FRAC = 8;

   // This is the input sample set. The samples are kept into an 
   reg [SAMPLES-1:0][WIDTH-1:0] src_real, src_imag;
   initial begin : LOAD
      integer fd;
      integer rc;
      real a, b;
      int     idx;
      reg [8*512-1:0] linebuf;
      $display("SAMPLES = %0d, WIDTH = %0d, FRAC = %0d", SAMPLES, WIDTH, FRAC);
      fd = $fopenr("comp_test_in.csv");
      idx = 0;
      while (idx < SAMPLES && ! $feof(fd) ) begin
	 rc = $fgets(linebuf, fd);
	 rc = $sscanf(linebuf, "%f, %f", a, b);
	 if (rc < 1) a = 0.0;
	 if (rc < 2) b = 0.0;
	 src_real[idx] = a * (1<<FRAC);
	 src_imag[idx] = b * (1<<FRAC);
	 idx = idx+1;
      end
      $fclose(fd);
      // Fill out remaining data with zeros.
      while (idx < SAMPLES) begin
	 src_real[idx] = 0;
	 src_imag[idx] = 0;
	 idx = idx+1;
      end
   end

   reg  clk;
   reg  reset;
   wire ready;

   // The output component.
   wire [WIDTH-1:0] dft_real, dft_imag;
   // Select the component to calculate
   reg [$clog2(SAMPLES):0] dft_idx;

   // This is the device under test.
   idft_slice #(.WIDTH(WIDTH), .FRAC(FRAC)) dut
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

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

# include  <complex>
# include  <string>
# include  <vector>
# include  <cstdio>
# include  <cstring>
# include  <cassert>

/*
 * This program generates a verilog module to calculate a slice of the
 * DFT of an input vector. The input is a complete vector of complex
 * numbers, and the output is one component of the DFT of the input
 * vector. The component selection is one of the inputs of the
 * module. The format of the generated module is:
 *
 * module NAME
 *   #(parameter WIDTH = WIDTH,
 *     parameter FRAC  = FRAC)
 *    (input                           clk,
 *     input                           reset,
 *     output                          ready,
 *     output signed [WIDTH-1:0]       dft_real, dft_imag,
 *     input     [$clog2(N)-1:0]       dft_idx,
 *     input signed [N-1:0][WIDTH-1:0] src_real, src_imag);
 *
 *     [...]
 *
 * endmodule
 *
 * The input clk is a free-running clock, and the reset restarts the
 * processing engine. Hold the reset high while setting up the other
 * inputs, including the src_real and src_imag vectors (which are the
 * real and imaginary components of the vector of complex values) and
 * dft_idx, which selects which DFT output component to
 * calculate. When these inputs are ready, set reset to low, and the
 * module will process until the output ready signal goes high. At
 * this point the dft_read and dft_imag are the complex value of the
 * output component selected by dft_idx.
 *
 * To calculate another component, set reset back to high, then change
 * any of the inputs, and lower the reset again to start the new
 * calculation. Each calculation takes about log2(N) clocks. A
 * complete output vector requires N calculations, so with only 1
 * instantiation, the full DFT takes O(N*log2(N)) to compute. If there
 * are enough hardware resources, the module can be instantiated many
 * times to process multiple components in parallel. (Each output
 * component depends only on the input vector and the output component
 * address.)
 *
 * The module NAME is given on the command line with the --name=<NAME>
 * command line flag, and N is given by the --N=<N> flag. The
 * generator is designed to assume that N==2**k where k is an
 * integer greater than 2. The output Verilog code is sent to
 * stdout. So for example:
 *
 *    ./idft_gen --name=idft_N32 -N=32 > idft_N32.v
 *
 * For a description of the algorithm used, see dft_recurse.cc and
 * idft_recurse.cc. The dft_recurse.cc program breaks down the FFT
 * algorithm and explains how it works, and idft_recurse.cc
 * demonstrates a fixed point version.
 */

# define TRANS_SIGN (1)
using namespace std;

static size_t clog2(size_t N)
{
      size_t result = 0;
      while (N > 1) {
	    N /= 2;
	    result += 1;
      }
      return result;
}

/*
 * Pack all the "w" constant factors into separate modules. All the W
 * constants depend only on N (or decomposed N) and the output
 * idx. Calculate a module W<N> for each N used by the main module,
 * that takes as input the output idx and emits the proper constant
 * value.
 *
 * The W module take in the dft_idx that selects the output slice, and
 * generates, one clock later, the W<N> constant. It does this by
 * looking up the value in a rom.
 */
void generate_W_modules(FILE*fd, size_t N, const char*base_name)
{
      assert(N >= 2);

      size_t use_n = N;
      while (use_n > 2) {
	    size_t use_n_log2 = clog2(use_n);
	    double Nd = use_n;
	    const complex<double> W = exp( complex<double>(0.0, TRANS_SIGN * 2*M_PI/Nd) );
	    fprintf(fd, "/* Table of pow(W[N=%zu], idx) values. */\n", use_n);
	    fprintf(fd, "module %s$W%zu\n", base_name, use_n);
	    fprintf(fd, "  #(parameter WIDTH = 24,\n");
	    fprintf(fd, "    parameter FRAC  =  8\n");
	    fprintf(fd, "    /* */)\n");
	    fprintf(fd, "   (input wire clk,\n");
	    fprintf(fd, "    input wire [%zu:0] sel,\n", use_n_log2-1);
	    fprintf(fd, "    output reg [WIDTH*2-1:0] w\n");
	    fprintf(fd, "    /* */);\n");
	    fprintf(fd, "    reg [WIDTH-1:0] w_creal [0 : %zu];\n", use_n-1);
	    fprintf(fd, "    reg [WIDTH-1:0] w_cimag [0 : %zu];\n", use_n-1);
	    fprintf(fd, "    initial begin\n");
	    fprintf(fd, "      w_creal[0] = 1 << FRAC;\n");
	    fprintf(fd, "      w_cimag[0] = 0;\n");
	    fprintf(fd, "      w_creal[1] = %f * (1 << FRAC);\n", W.real());
	    fprintf(fd, "      w_cimag[1] = %f * (1 << FRAC);\n", W.imag());
	    for (size_t idx = 2 ; idx < use_n ; idx += 1) {
		  const complex<double> tmp = pow(W, idx);
		  fprintf(fd, "      // pow(W,%zu) = (%10.6f %10.6f)\n",
			  idx, tmp.real(), tmp.imag());
		  fprintf(fd, "      w_creal[%zu] = %f * (1 << FRAC);\n", idx, tmp.real());
		  fprintf(fd, "      w_cimag[%zu] = %f * (1 << FRAC);\n", idx, tmp.imag());
	    }
	    fprintf(fd, "    end\n");
	    fprintf(fd, "    always @(posedge clk)\n");
	    fprintf(fd, "      w <= {w_creal[sel], w_cimag[sel]};\n");
	    fprintf(fd, "endmodule\n");
	    use_n /= 2;
      }
}

/*
 * The blend module calculates the expression o = a + w*b, where all
 * the numbers are complex. The multiplications are registered, but
 * all the additions are combinational, so the expression takes one
 * clock to calculate.
 */
void generate_blend_module(FILE*fd, size_t N, const char*base_name)
{
      fprintf(fd, "/* This module implements the complex calculation: o = a + w*b */\n");
      fprintf(fd, "module %s$blend_math\n", base_name);
      fprintf(fd, "  #(parameter WIDTH = 24,\n");
      fprintf(fd, "    parameter FRAC  =  8\n");
      fprintf(fd, "    /* */)\n");
      fprintf(fd, "   (input  wire clk,\n");
      fprintf(fd, "    input  wire reset,\n");
      fprintf(fd, "    input  wire in_ready,\n");
      fprintf(fd, "    output reg  out_ready,\n");
      fprintf(fd, "    output wire signed [WIDTH-1:0] o_real,\n");
      fprintf(fd, "    output wire signed [WIDTH-1:0] o_imag,\n");
      fprintf(fd, "    input  wire signed [WIDTH-1:0] a_real,\n");
      fprintf(fd, "    input  wire signed [WIDTH-1:0] a_imag,\n");
      fprintf(fd, "    input  wire signed [WIDTH-1:0] w_real,\n");
      fprintf(fd, "    input  wire signed [WIDTH-1:0] w_imag,\n");
      fprintf(fd, "    input  wire signed [WIDTH-1:0] b_real,\n");
      fprintf(fd, "    input  wire signed [WIDTH-1:0] b_imag\n");
      fprintf(fd, "    /* */);\n");
      fprintf(fd, "\n");
      fprintf(fd, "    reg [WIDTH*2-1:0] wr_br;\n");
      fprintf(fd, "    always @(posedge clk) wr_br <= w_real * b_real;\n");
      fprintf(fd, "    reg [WIDTH*2-1:0] wi_bi;\n");
      fprintf(fd, "    always @(posedge clk) wi_bi <= w_imag * b_imag;\n");
      fprintf(fd, "    reg [WIDTH*2-1:0] wr_bi;\n");
      fprintf(fd, "    always @(posedge clk) wr_bi <= w_real * b_imag;\n");
      fprintf(fd, "    reg [WIDTH*2-1:0] wi_br;\n");
      fprintf(fd, "    always @(posedge clk) wi_br <= w_imag * b_real;\n");
      fprintf(fd, "    assign o_real = a_real + (wr_br>>>FRAC) - (wi_bi>>>FRAC);\n");
      fprintf(fd, "    assign o_imag = a_imag + (wr_bi>>>FRAC) + (wi_br>>>FRAC);\n");
      fprintf(fd, "    always @(posedge clk)\n");
      fprintf(fd, "       if (reset) out_ready <= 1'b0;\n");
      fprintf(fd, "       else       out_ready <= in_ready;\n");
      fprintf(fd, "endmodule\n");
}

std::string idft_math_recursive_gen(FILE*fd, const size_t N, const char*base_name, const std::vector<size_t>&src)
{
      char out_name[512];
      snprintf(out_name, sizeof out_name, "p_N%zu_s%zu", N, src[0]);
      if (src.size() == 2) {
	      // Tail of the recursive splitting of the problem. At
	      // this point, the only inputs are the raw source data,
	      // and some small literal constants. This step takes one
	      // clock to process, so the level N2 data is ready one
	      // clock after the reset is released.
	    fprintf(fd, "     // %s == src[%zu] + W2[idx] * src[%zu] (tail)\n",
		    out_name, src[0], src[1]);
	    char a_name[512];
	    snprintf(a_name, sizeof a_name, "a_N%zu_s%zu", N, src[0]);
	    char b_name[512];
	    snprintf(b_name, sizeof b_name, "b_N%zu_s%zu", N, src[0]);

	    fprintf(fd, "    wire complex_t %s = {src_real[%zu], src_imag[%zu]};\n", a_name, src[0], src[0]);
	    fprintf(fd, "    wire complex_t %s = {src_real[%zu], src_imag[%zu]};\n", b_name, src[1], src[1]);

	    fprintf(fd, "    reg complex_t %s;\n", out_name);
	    fprintf(fd, "    reg           %s_ready;\n", out_name);
	    fprintf(fd, "    always @(posedge clk) begin\n");
	    fprintf(fd, "       %s.creal <= %s.creal + (dft_idx[0]? -%s.creal : %s.creal);\n",
		    out_name, a_name, b_name, b_name);
	    fprintf(fd, "       %s.cimag <= %s.cimag + %s.cimag;\n",
		    out_name, a_name, b_name);
	    fprintf(fd, "       if (reset) %s_ready <= 1'b0;\n", out_name);
	    fprintf(fd, "       else       %s_ready <= 1'b1;\n", out_name);
	    fprintf(fd, "    end\n");
	    return out_name;
      }

      vector<size_t> src_e, src_o;
      src_e.resize(N/2);
      src_o.resize(N/2);
      for (size_t idx = 0 ; idx < N/2 ; idx += 1) {
	    src_e[idx] = src[2*idx+0];
	    src_o[idx] = src[2*idx+1];
      }

      string e = idft_math_recursive_gen(fd, N/2, base_name, src_e);
      string o = idft_math_recursive_gen(fd, N/2, base_name, src_o);
      char w_name[512];
      snprintf(w_name, sizeof w_name, "w_N%zu_s%zu", N, src[0]);
      fprintf(fd, "     // %s ==  %s + pow(W%zu, idx) * %s (blend)\n",
	      out_name, e.c_str(), N, o.c_str());
      fprintf(fd, "    wire complex_t %s;\n", out_name);
      fprintf(fd, "    wire complex_t %s;\n", w_name);
      fprintf(fd, "    %s$W%zu #(.WIDTH(WIDTH), .FRAC(FRAC)) %s_table(clk, dft_idx[%zu:0], %s);\n",
	      base_name, N, w_name, clog2(N)-1, w_name);
      fprintf(fd, "    wire %s_in_ready = %s_ready & %s_ready;\n", out_name, e.c_str(), o.c_str());
      fprintf(fd, "    wire %s_ready;\n", out_name);
      fprintf(fd, "    %s$blend_math #(.WIDTH(WIDTH), .FRAC(FRAC)) %s_math\n",
	      base_name, out_name);
      fprintf(fd, "        (clk, reset, %s_in_ready, %s_ready,\n", out_name, out_name);
      fprintf(fd, "         %s.creal, %s.cimag,\n", out_name, out_name);
      fprintf(fd, "         %s.creal, %s.cimag,\n", e.c_str(), e.c_str());
      fprintf(fd, "         %s.creal, %s.cimag,\n", w_name, w_name);
      fprintf(fd, "         %s.creal, %s.cimag);\n", o.c_str(), o.c_str());
      return out_name;
}

void generate_module(FILE*fd, size_t N, const char*base_name)
{
      size_t Nlog2 = clog2(N);

      fprintf(fd, "/*\n");
      fprintf(fd, " * This code generated by idft_gen. Do not edit.\n");
      fprintf(fd, " */\n");
      fprintf(fd, "\n");
      generate_W_modules(fd, N, base_name);
      fprintf(fd, "\n");
      generate_blend_module(fd, N, base_name);
      fprintf(fd, "\n");
      fprintf(fd, "module %s  /* N (number of samples) = %zu */\n", base_name, N);
      fprintf(fd, "  #(parameter WIDTH = 24,\n");
      fprintf(fd, "    parameter FRAC  = 8\n");
      fprintf(fd, "    /* */)\n");
      fprintf(fd, "   (input wire  clk,\n");
      fprintf(fd, "    input wire  reset,\n");
      fprintf(fd, "    output wire ready,\n");
      fprintf(fd, "    // Single output component of DFT\n");
      fprintf(fd, "    output wire signed [WIDTH-1:0] dft_real, dft_imag,\n");
      fprintf(fd, "    // Which output component are we calculating?\n");
      fprintf(fd, "    input wire [%zu:0] dft_idx,\n", Nlog2-1);
      fprintf(fd, "    // ector of input samples\n");
      fprintf(fd, "    input wire signed [%zu:0][WIDTH-1:0] src_real, src_imag\n", N-1);
      fprintf(fd, "    /* */);\n");

      fprintf(fd, "    typedef struct packed {\n");
      fprintf(fd, "        logic signed [WIDTH-1:0] creal;\n");
      fprintf(fd, "        logic signed [WIDTH-1:0] cimag;\n");
      fprintf(fd, "    } complex_t;\n");

      vector<size_t> src;
      src.resize(N);
      for (size_t idx = 0 ; idx < N ; idx += 1)
	    src[idx] = idx;
      string res = idft_math_recursive_gen(fd, N, base_name, src);

      fprintf(fd, "\n");
      fprintf(fd, "    assign dft_real = %s.creal;\n", res.c_str());
      fprintf(fd, "    assign dft_imag = %s.cimag;\n", res.c_str());
      fprintf(fd, "    assign ready    = %s_ready;\n", res.c_str());
      fprintf(fd, "endmodule\n");
}

int main(int argc, char*argv[])
{
      size_t N = 32;
      const char*name = "idft_comp";
      FILE*fd = stdout;

      for (int idx = 1 ; idx < argc ; idx += 1) {
	    if (strncmp(argv[idx], "--name=", 7) == 0) {
		  name = argv[idx]+7;

	    } else if (strncmp(argv[idx], "--N=", 4) == 0) {
		  N = strtoul(argv[idx]+4, 0, 0);

	    } else {
	    }
      }
      
      generate_module(fd, N, name);
      
      return 0;
}

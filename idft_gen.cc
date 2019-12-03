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

# define TRANS_SIGN (1)
using namespace std;

void generate_W_tables(FILE*fd, size_t N)
{
      assert(N >= 2);

      fprintf(fd, "    typedef struct packed {\n");
      fprintf(fd, "        logic signed [WIDTH-1:0] creal;\n");
      fprintf(fd, "        logic signed [WIDTH-1:0] cimag;\n");
      fprintf(fd, "    } complex_t;\n");
      size_t use_n = N;
      while (use_n > 2) {
	    double Nd = use_n;
	    const complex<double> W = exp( complex<double>(0.0, TRANS_SIGN * 2*M_PI/Nd) );

	    fprintf(fd, "    reg complex_t [%zu:0] W%zu;\n", use_n-1, use_n);
	    fprintf(fd, "    initial begin\n");
	    fprintf(fd, "      W%zu[0].creal = 1 << FRAC;\n", use_n);
	    fprintf(fd, "      W%zu[0].cimag = 0;\n", use_n);
	    fprintf(fd, "      W%zu[1].creal = %f * (1 << FRAC);\n",
		    use_n, W.real());
	    fprintf(fd, "      W%zu[1].cimag = %f * (1 << FRAC);\n",
		    use_n, W.imag());
	    for (size_t idx = 2 ; idx < use_n ; idx += 1) {
		  const complex<double> tmp = pow(W, idx);
		  fprintf(fd, "      // pow(W,%zu) = (%10.6f %10.6f)\n",
			  idx, tmp.real(), tmp.imag());
		  fprintf(fd, "      W%zu[%zu].creal = %f * (1 << FRAC);\n",
			  use_n, idx, tmp.real());
		  fprintf(fd, "      W%zu[%zu].cimag = %f * (1 << FRAC);\n",
			  use_n, idx, tmp.imag());
	    }
	    fprintf(fd, "    end\n");
	    use_n /= 2;
      }

}

std::string idft_math_recursive_gen(FILE*fd, const size_t N, const std::vector<size_t>&src)
{
      char out_name[512];
      snprintf(out_name, sizeof out_name, "p_N%zu_s%zu", N, src[0]);
      if (src.size() == 2) {
	    fprintf(fd, "     // %s == src[%zu] + W2[idx] * src[%zu] (tail)\n",
		    out_name, src[0], src[1]);
	    char a_name[512];
	    snprintf(a_name, sizeof a_name, "a_N%zu_s%zu", N, src[0]);
	    char b_name[512];
	    snprintf(b_name, sizeof b_name, "b_N%zu_s%zu", N, src[0]);

	    fprintf(fd, "    wire complex_t %s = {src_real[%zu], src_imag[%zu]};\n", a_name, src[0], src[0]);
	    fprintf(fd, "    wire complex_t %s = {src_real[%zu], src_imag[%zu]};\n", b_name, src[1], src[1]);

	    fprintf(fd, "    wire complex_t %s;\n", out_name);
	    fprintf(fd, "    assign %s.creal = %s.creal + (dft_idx[0]? -%s.creal : %s.creal);\n",
		    out_name, a_name, b_name, b_name);
	    fprintf(fd, "    assign %s.cimag = %s.cimag + %s.cimag;\n",
		    out_name, a_name, b_name);
	    fprintf(fd, "    reg %s_ready;\n", out_name);
	    fprintf(fd, "    always @(posedge clk)\n");
	    fprintf(fd, "      if (reset) %s_ready <= 1'b0;\n", out_name);
	    fprintf(fd, "      else       %s_ready <= 1'b1;\n", out_name);
	    return out_name;
      }

      vector<size_t> src_e, src_o;
      src_e.resize(N/2);
      src_o.resize(N/2);
      for (size_t idx = 0 ; idx < N/2 ; idx += 1) {
	    src_e[idx] = src[2*idx+0];
	    src_o[idx] = src[2*idx+1];
      }

      string e = idft_math_recursive_gen(fd, N/2, src_e);
      string o = idft_math_recursive_gen(fd, N/2, src_o);
      char w_name[512];
      snprintf(w_name, sizeof w_name, "w_N%zu_s%zu", N, src[0]);
      fprintf(fd, "     // %s ==  %s + pow(W%zu, idx) * %s (blend)\n",
	      out_name, e.c_str(), N, o.c_str());
      fprintf(fd, "    wire complex_t %s;\n", out_name);
      fprintf(fd, "    wire complex_t %s = W%zu[dft_idx];\n", w_name, N);
      fprintf(fd, "    wire %s_in_ready = %s_ready & %s_ready;\n", out_name, e.c_str(), o.c_str());
      fprintf(fd, "    wire %s_ready;\n", out_name);
      fprintf(fd, "    blend_math #(.WIDTH(WIDTH), .FRAC(FRAC)) %s_math\n", out_name);
      fprintf(fd, "        (clk, reset, %s_in_ready, %s_ready,\n", out_name, out_name);
      fprintf(fd, "         %s.creal, %s.cimag,\n", out_name, out_name);
      fprintf(fd, "         %s.creal, %s.cimag,\n", e.c_str(), e.c_str());
      fprintf(fd, "         %s.creal, %s.cimag,\n", w_name, w_name);
      fprintf(fd, "         %s.creal, %s.cimag);\n", o.c_str(), o.c_str());
      return out_name;
}

static size_t clog2(size_t N)
{
      size_t result = 0;
      while (N > 1) {
	    N /= 2;
	    result += 1;
      }
      return result;
}

void generate_module(FILE*fd, size_t N)
{
      size_t Nlog2 = clog2(N);

      fprintf(fd, "/*\n");
      fprintf(fd, " * This code generated by idft_gen. Do not edit.\n");
      fprintf(fd, " */\n");
      fprintf(fd, "\n");
      fprintf(fd, "module blend_math\n");
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
      fprintf(fd, "\n");
      fprintf(fd, "module idft_comp_N%zu  /* N (number of samples) = %zu */\n", N, N);
      fprintf(fd, "  #(parameter WIDTH = 24,\n");
      fprintf(fd, "    parameter FRAC  = 8\n");
      fprintf(fd, "    /* */)\n");
      fprintf(fd, "   (input wire clk,\n");
      fprintf(fd, "    input wire reset,\n");
      fprintf(fd, "    output reg ready,\n");
      fprintf(fd, "    // Single output component of DFT\n");
      fprintf(fd, "    output reg signed [WIDTH-1:0] dft_real, dft_imag,\n");
      fprintf(fd, "    // Which output component are we calculating?\n");
      fprintf(fd, "    input wire [%zu:0] dft_idx,\n", Nlog2-1);
      fprintf(fd, "    // ector of input samples\n");
      fprintf(fd, "    input wire signed [%zu:0][WIDTH-1:0] src_real, src_imag\n", N-1);
      fprintf(fd, "    /* */);\n");

      generate_W_tables(fd, N);

      vector<size_t> src;
      src.resize(N);
      for (size_t idx = 0 ; idx < N ; idx += 1)
	    src[idx] = idx;
      string res = idft_math_recursive_gen(stdout, N, src);

      fprintf(fd, "    always @(posedge clk)\n");
      fprintf(fd, "     if (reset) begin\n");
      fprintf(fd, "       dft_real <= 0;\n");
      fprintf(fd, "       dft_imag <= 0;\n");
      fprintf(fd, "       ready    <= 1'b0;\n");
      fprintf(fd, "     end else begin\n");
      fprintf(fd, "       dft_real <= %s.creal;\n", res.c_str());
      fprintf(fd, "       dft_imag <= %s.cimag;\n", res.c_str());
      fprintf(fd, "       ready    <= %s_ready;\n", res.c_str());
      fprintf(fd, "    end\n");
      fprintf(fd, "endmodule\n");
}

int main(int argc, char*argv[])
{
      size_t N = 32;
      FILE*fd = stdout;

      for (int idx = 1 ; idx < argc ; idx += 1) {
	    if (strncmp(argv[idx], "--N=", 4) == 0) {
		  N = strtoul(argv[idx]+4, 0, 0);

	    } else {
	    }
      }
      
      generate_module(fd, N);
      
      return 0;
}

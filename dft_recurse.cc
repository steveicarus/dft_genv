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

# include  "priv.h"
# include  <complex>
# include  <iostream>
# include  <vector>
# include  <cmath>
# include  <cstdio>
# include  <cstring>

# include  <cassert>

# define TRANS_SIGN (1)

using namespace std;

/*
 * The recursive version (Danielson-Lanczos method) works by
 * calculating the DFT of the even subset of a vector, the odd subset
 * of a vector, then mixing the result together. This algorithm is
 * assuming that the original input vector is size N == 2**k for some k.
 *
 * The dft_recurse() function calculates the DFT(src) for the input
 * vector, and returns a vector. The do_recurse2 function calculates
 * the DFT[idx](src) for a vector, and returns a scaler that is the
 * idx element of the DFT vector. DFT[idx] is called by dft_recurse()
 * for each element of the output vector to build up the DFT.
 */
static complex<double> do_recurse2(size_t idx, const std::vector< complex<double> >&src)
{
	// We need double and size_t versions of N.
      const double Nd = src.size();
      const size_t Ni = src.size();

      assert(Ni%2 == 0);
      assert(Ni >= 2);

      if (Ni == 2) {
	      // The DFT[idx] of a 2-item vector is a special case. We
	      // need not recurse any further, and also the two-term
	      // expression to calculate the value reduces a lot. To wit:
	      //
	      //   src[0] * exp( 2*pi*i*0*(idx)/2 ) + src[1] * exp( 2*pi*i*1*(idx)/2 )
	      //   src[0] * 1                       + src[1] * exp( pi*i*(idx) )
	      //
	      // So the function exp( pi*i*idx )is this:
	      //
	      //   Widx = exp( complex<double>(0.0, M_PI*idx) );
	      //
	      // But wait! This is periodic with period 2, so we only
	      // need two values:
	      //
	      //    Widx = exp( complex<double>(0.0, M_PI*(idx%2)) );
	      //
	      // But there's more! exp(complex<double>(0.0, 0.0)) is +1,
	      // and exp(compex<double>(0.0, M_PI)) is -1, so we can
	      // reduce this even further, to the rather trivial
	      // expression below.
	    const complex<double> Widx = complex<double>((idx%2)==0? 1.0 : -1.0, 0.0);
	    return src[0] + Widx * src[1];
      }

	// The DFT[idx](src) of a N-item vector is:
	//
	//   SUM[j=0, N/2-1] ( src[2*j] * exp( 2*pi*i*j*(idx)/(N/2) ) )
	//     + pow(exp( 2pi*i/N ), idx) * SUM[j=0, N/2-1] ( src[2*j+1] * exp( 2*pi*i*j*(idx)/(N/2) ) )
	//
	// Since the SUM[...] terms are DFT[idx](...) of sub-vectors
	// src_odd and src_even, we can write this as a recursive
	// function:
	//
	//  src_e = src[0, 2, 4, ..., N-2];
	//  src_o = src[1, 3, 5, ..., N-1];
	//  DFT[idx](src) = DFT[idx](src_ev) + pow(exp( 2*pi*i/N ), idx) * DFT[idx](src_odd)

      vector< complex<double> > src_e, src_o;
      src_e.resize(Ni/2);
      src_o.resize(Ni/2);
      for (size_t j = 0 ; j < Ni/2 ; j += 1) {
	    src_e[j] = src[2*j+0];
	    src_o[j] = src[2*j+1];
      }

      const complex<double> W = exp( complex<double>(0.0, TRANS_SIGN * 2*M_PI/Nd) );
      const complex<double> Wk = pow(W, idx%Ni);
      return do_recurse2(idx, src_e) + Wk * do_recurse2(idx, src_o);
}

std::vector< complex<double> > dft_recurse(const std::vector< complex<double> >&src)
{
	// We need double and size_t versions of N.
	//const double Nd = src.size();
      const size_t Ni = src.size();

	//const complex<double> W1 = exp( complex<double>(0.0, 2*M_PI)/Nd );

      vector< complex<double> > dst;
      dst.resize(Ni);

	// The DFT(src) is a vector with N components. Build up the
	// vector by calculations DFT[idx] for 0 <= idx < N to build
	// up the vector result.
      for (size_t idx = 0 ; idx < Ni ; idx += 1) {
	    dst[idx] = do_recurse2(idx, src);
      }

      return dst;
}


int main(int argc, char*argv[])
{
      const char*src_path = 0;
      const char*dst_path = 0;

      for (int idx = 1 ; idx < argc ; idx += 1) {
	    if (strncmp(argv[idx],"--src=",6) == 0) {
		  src_path = argv[idx]+6;

	    } else if (strncmp(argv[idx],"--dst=",6) == 0) {
		  dst_path = argv[idx]+6;

	    } else {
	    }
      }

      FILE*src_fd = fopen(src_path, "rt");
      if (src_fd == 0) {
	    fprintf(stderr, "Unable to open input file %s\n", src_path);
	    return -1;
      }
      
      vector< complex<double> > src = read_values_d(src_fd);
      assert(src.size() != 0);

      fclose(src_fd);
      src_fd = 0;

      vector< complex<double> > dst = dft_recurse(src);

      FILE*dst_fd = fopen(dst_path, "wt");
      if (dst_fd == 0) {
	    fprintf(stderr, "Unable to open output file %s\n", dst_path);
	    return -1;
      }

      write_values(dst_fd, dst);
      return 0;
}

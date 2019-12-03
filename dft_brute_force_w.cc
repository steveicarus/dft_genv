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
 * This is still the brute force method, but pre-calculate the powers
 * of W that will be needed. This makes sense since all the various
 * powers of W do not depend on the input data at all. Also, we can
 * take advantage of the fact that W**x is itself harmonic, with
 * period of N.
 */

static std::vector< complex<double> > precalculate_W(const size_t N)
{
	// As an optimization, we can precalculate the W values that
	// we will need. We only need a small number of values since
	// we we can take advantage of symmetries. In particular:
	//
	//    W[0] == 1.0             -- Anything to 0-th power is 1
	//    W[1] = exp( -2*PI*i/N ) -- 
	//    W**(kN + j) == W**j     -- This means we only need N values.
      assert(N > 0);
      vector< complex<double> > W;
      W.resize(N);
      W[0] = 1.0;  if (N == 1) return W;
      W[1] = exp( complex<double>(0.0, TRANS_SIGN * 2*M_PI)/(double)N );
      for (size_t idx = 2 ; idx < N ; idx += 1) {
	    W[idx] = pow(W[1], idx);
      }

      return W;
}

/*
 * Implement the DFT using brute force. If f[n] is the input vector,
 * and F[n] the output vector, and there are N elements in the input
 * and output vectors, the formula being calculated is:
 *
 *      F[n] = SUM[k=0 to N-1]( f[k] * (W ** (n*k)) )
 *
 * where:
 *
 *      W = exp( (2*pi*i)/N )
 *
 * This function run time is O(N**2), which is not practical; but this
 * makes for a workable reference implementation.
 */
std::vector< complex<double> > dft_brute_force_w(const std::vector< complex<double> >&src)
{
	// We need double and size_t versions of N.
	//const double Nd = src.size();
      const size_t Ni = src.size();

	// Precalculate W**0 ... W**(N-1)
      const vector< complex<double> > W = precalculate_W(Ni);

      vector< complex<double> > dst;
      dst.resize(Ni);

	// Iterate over the output vector components.
      for (size_t idx = 0 ; idx < Ni ; idx += 1) {
	      // We know that W[0]==1.0, so save a multiply.
	    dst[idx] = src[0];
	    for (size_t k = 1 ; k < Ni ; k += 1) {
		  dst[idx] += src[k] * W[(idx*k)%Ni];
	    }
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

      vector< complex<double> > dst = dft_brute_force_w(src);

      FILE*dst_fd = fopen(dst_path, "wt");
      if (dst_fd == 0) {
	    fprintf(stderr, "Unable to open output file %s\n", dst_path);
	    return -1;
      }

      write_values(dst_fd, dst);
      return 0;
}

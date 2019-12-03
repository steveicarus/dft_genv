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
# include  "icomplex.h"
# include  <complex>
# include  <vector>
# include  <cstdio>
# include  <cstring>
# include  <cassert>

using namespace std;

# define TRANS_SIGN (1)
//# define TRANS_SIGN (-1)

# define USE_FRAC_BITS 8
typedef icomplex<int32_t,USE_FRAC_BITS> use_icomplex_t;

std::vector<use_icomplex_t> idft_brute_force(const std::vector<use_icomplex_t>&src)
{
      const int Ni = src.size();
      const double Nd = src.size();

      vector<use_icomplex_t> dst;
      dst.resize(Ni);

      complex<double> W1 = exp( complex<double>(0.0, TRANS_SIGN * 2*M_PI)/Nd );
      vector<use_icomplex_t> W;
      W.resize(Ni);
      W[0] = 1.0;
      W[1] = W1;
      for (int k = 2 ; k < Ni ; k += 1) {
	    W[k] = pow(W1, k);
      }
      
      for (int idx = 0 ; idx < Ni ; idx += 1) {
	    dst[idx] = src[0];
	    for (int k = 1 ; k < Ni ; k += 1) {
		  dst[idx] += src[k] * W[(idx*k) % Ni];
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
      
      vector< complex<double> > src_raw = read_values_d(src_fd);
      assert(src_raw.size() != 0);

      fclose(src_fd);
      src_fd = 0;

      vector<use_icomplex_t> src;
      src.resize(src_raw.size());
      for (size_t idx = 0 ; idx < src.size() ; idx += 1)
	    src[idx] = src_raw[idx];

      vector<use_icomplex_t> dst = idft_brute_force(src);

      FILE*dst_fd = fopen(dst_path, "wt");
      if (dst_fd == 0) {
	    fprintf(stderr, "Unable to open output file %s\n", dst_path);
	    return -1;
      }

      for (size_t idx = 0 ; idx < src.size() ; idx += 1) {
	    fprintf(dst_fd, "0x%08x, 0x%08x,  %lf, %lf\n",
		    dst[idx].real(), dst[idx].imag(),
		    dst[idx].d_real(), dst[idx].d_imag());
      }

      fclose(dst_fd);
      return 0;
}

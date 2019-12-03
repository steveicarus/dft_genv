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

# include  <fftw3.h>

# include  <cassert>

# define TRANS_SIGN (1)

/*
 * This is a reference implementation that uses the FFTW3
 * library. It's  black box that I can test other attempts against.
 */

using namespace std;

std::vector< complex<double> > dft_fftw3 (const std::vector< complex<double> >&src)
{
	// format the source vector into the scratch space that the
	// fftw library can work with.
      fftw_complex*spc = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * src.size());

	// Let FFTW3 figure out how it's going to do its work.
      fftw_plan plan = fftw_plan_dft_1d(src.size(), spc, spc,
					FFTW_BACKWARD, FFTW_ESTIMATE);

	// Copy the input vector to the scratch space.
      for (size_t idx = 0 ; idx < src.size() ; idx += 1) {
	    spc[idx][0] = src[idx].real();
	    spc[idx][1] = src[idx].imag();
      }

	// Make it so.
      fftw_execute(plan);

	// Transfer the results out to the destination vector.
      vector< complex<double> > dst;
      dst.resize(src.size());
       for (size_t idx = 0 ; idx < dst.size() ; idx += 1) {
	    dst[idx] = complex<double>(spc[idx][0], spc[idx][1]);
      }

      fftw_destroy_plan(plan);
      fftw_free(spc);
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

      vector< complex<double> > dst = dft_fftw3(src);

      FILE*dst_fd = fopen(dst_path, "wt");
      if (dst_fd == 0) {
	    fprintf(stderr, "Unable to open output file %s\n", dst_path);
	    return -1;
      }

      write_values(dst_fd, dst);
      return 0;
}

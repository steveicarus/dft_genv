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
# include  <vector>
# include  <cstdio>
# include  <cassert>

/*
 * Read in a vector of complex numbers. The numbers are stored as
 * comma separated values (real part, imaginary part) one number per
 * line.
 */

using namespace std;

std::vector< complex<double> > read_values_d(FILE*fd)
{
      vector< complex<double> > result;
      char linebuf[512];
      while (fgets(linebuf, sizeof linebuf, fd) != 0) {
	    double a, b;
	    int rc = sscanf(linebuf, "%lf , %lf", &a, &b);
	    switch (rc) {
		case 0:
		  break;
		case 1:
		  result.push_back( complex<double>(a, 0.0) );
		  break;
		case 2:
		  result.push_back( complex<double>(a, b) );
		  break;
		default:
		  assert(0);
		  break;
	    }
      }

      return result;
}

void write_values(FILE*fd, const std::vector< complex<double> >&data)
{
      for (size_t idx = 0 ; idx < data.size() ; idx += 1) {
	    fprintf(fd, "%lf, %lf\n", data[idx].real(), data[idx].imag());
      }
}

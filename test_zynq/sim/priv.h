#ifndef __priv_H
#define __priv_H
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
# include  <vector>

/*
 * Read a vector of values from a comma separated values (CSV)
 * file. Read to the end of the file.
 */
extern std::vector< std::complex<double> > read_values_d(FILE*fd);

/*
 * Write a vector of values to a CSV file. This is the same format
 * that the reader can read.
 */
extern void write_values(FILE*fd, const std::vector< std::complex<double> >&data);

#endif

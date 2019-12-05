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

/*
 * This is the simulation engine for the SandyLinux sandbox AXI device.
 */

# include  "priv.h"

# define _STDC_FORMAT_MACROS
# include  <complex>
# include  <vector>
# include  <simbus_axi4.h>
# include  <cassert>

using namespace std;

const char*port_string = "pipe:dft_master.pipe";
const unsigned dft_addr_width = 24;

/*
 * These are addresses on the AXI4 bus
 */
const uint32_t DFT_CmdStatus = 0x000000;
const uint32_t DFT_IDX       = 0x000004;
const uint32_t DFT_Config    = 0x000008;
const uint32_t DFT_REAL      = 0x000010;
const uint32_t DFT_IMAG      = 0x000014;
const uint32_t DFT_BUILD     = 0x000100;
const uint32_t SRC_REAL      = 0x800000;
const uint32_t SRC_IMAG      = 0xc00000;


uint32_t dft_read32(simbus_axi4_t bus, uint64_t addr)
{
      uint32_t val;
      simbus_axi4_resp_t axi4_rc = simbus_axi4_read32(bus, addr, 0x00, &val);
      return val;
}

void dft_write32(simbus_axi4_t bus, uint64_t addr, uint32_t data)
{
      simbus_axi4_resp_t axi4_rc = simbus_axi4_write32(bus, addr, 0x00, data);
}


int main(int argc, char*argv[])
{
      int rc;
      simbus_axi4_t bus = simbus_axi4_connect(port_string, "master",
					      32, dft_addr_width, 4, 4, 1);
      assert(bus);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      printf("Reset bus...\n");
      fflush(stdout);
      simbus_axi4_reset(bus, 8, 8);

      uint32_t build_id = dft_read32(bus, DFT_BUILD);
      printf("FPGA BUILD = %" PRIu32 "\n", build_id);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      uint32_t dft_config = dft_read32(bus, DFT_Config);
      unsigned int dft_samples = (dft_config >> 16) & 0xffff;
      unsigned int dft_width   = (dft_config >>  8) & 0xff;
      unsigned int dft_frac    = (dft_config >>  0) & 0xff;
      printf("DFT Config = %08" PRIx32 " (SAMPLES=%u, WIDTH=%u, FRAC=%u)\n",
	     dft_config, dft_samples, dft_width, dft_frac);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      uint32_t dft_cmd_status = dft_read32(bus, DFT_CmdStatus);
      printf("DFT CmdStatus = 0x%08" PRIx32 "\n", dft_cmd_status);

      printf("Wait 8 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 8, 0);

      FILE*src_fd = fopen("dft_input.csv", "rt");
      assert(src_fd);
      vector< complex<double> >src = read_values_d(src_fd);
      fclose(src_fd);

	// Write the source data into the data memory. The source data
	// is written into the FPGA while the internal processing
	// "reset" is held active in the CmdStatus register.
      for (size_t idx = 0 ; idx < src.size() && idx < dft_samples ; idx += 1) {
	    uint32_t tmp = src[idx].real() * (1<<dft_frac) + 0.5;
	    dft_write32(bus, SRC_REAL + 4*idx, tmp);
      }
      for (size_t idx = 0 ; idx < src.size() && idx < dft_samples ; idx += 1) {
	    uint32_t tmp = src[idx].imag() * (1<<dft_frac) + 0.5;
	    dft_write32(bus, SRC_IMAG + 4*idx, tmp);
      }

      for (size_t idx = src.size() ; idx < dft_samples ; idx += 1)
	    dft_write32(bus, SRC_REAL + 4*idx, 0);
      for (size_t idx = src.size() ; idx < dft_samples ; idx += 1)
	    dft_write32(bus, SRC_IMAG + 4*idx, 0);

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

	// Given the source vector written into the FPGA, perform the
	// processing by generating one component of the result at a
	// time. Generate all N components to get the complete DFT.
      struct cpair_s { uint32_t real, imag; };
      vector<cpair_s> dst;
      dst.resize(dft_samples);
      for (size_t idx = 0 ; idx < dst.size() ; idx += 1) {
	      // Set the reset bit...
	    dft_write32(bus, DFT_CmdStatus, 1);
	      // Select the output we want to calculate
	    dft_write32(bus, DFT_IDX, idx);
	      // and clear the reset bit.
	    dft_write32(bus, DFT_CmdStatus, 0);

	      // For smaller DFT vectors (i.e. 128 or so) the
	      // processing is so fast that it practically finishes in
	      // the time it takes to read the status. But put these
	      // wait clocks in here to be a bit more realistic.
	    printf("Wait 4 clocks...\n");
	    fflush(stdout);
	    simbus_axi4_wait(bus, 4, 0);

	      // After the processing is complete, the ready bit (bit
	      // [1]) is set. Read the status to verify it, but again,
	      // things go so fast, we'll probably not see it false.
	    do {
		  dft_cmd_status = dft_read32(bus, DFT_CmdStatus);
		  printf("DFT CmdStatus = 0x%08" PRIx32 "\n", dft_cmd_status);
	    } while ((dft_cmd_status & 0x02) == 0);

	    dst[idx].real = dft_read32(bus, DFT_REAL);
	    dst[idx].imag = dft_read32(bus, DFT_IMAG);
      }

	// Dump the DFT that we got back from the hardware.
      for (size_t idx = 0 ; idx < dst.size() ; idx += 1) {
	    printf("DFT[%2zu]: 0x%08x 0x%08x\n", idx,
		   dst[idx].real, dst[idx].imag);
      }
      
      printf("Wait 8 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 8, 0);

      simbus_axi4_end_simulation(bus);
      return 0;
}

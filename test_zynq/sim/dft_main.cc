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

      vector< complex<double> >src;
      src.resize(32);
      src[0] = 1.0;
      src[1] = 1.0;
      src[2] = 1.0;
      src[3] = 1.0;
      src[4] = 1.0;
      src[5] = 1.0;
      src[6] = 1.0;
      src[7] = 1.0;
      src[8] = -1.0;
      src[9] = -1.0;
      src[10] = -1.0;
      src[11] = -1.0;
      src[12] = -1.0;
      src[13] = -1.0;
      src[14] = -1.0;
      src[15] = -1.0;
      src[16] = 1.0;
      src[17] = 1.0;
      src[18] = 1.0;
      src[19] = 1.0;
      src[20] = 1.0;
      src[21] = 1.0;
      src[22] = 1.0;
      src[23] = 1.0;
      src[24] = -1.0;
      src[25] = -1.0;
      src[26] = -1.0;
      src[27] = -1.0;
      src[28] = -1.0;
      src[29] = -1.0;
      src[30] = -1.0;
      src[31] = -1.0;

	// Write the source data into the data memory.
      for (size_t idx = 0 ; idx < src.size() ; idx += 1) {
	    uint32_t tmp = src[idx].real() * (1<<dft_frac) + 0.5;
	    dft_write32(bus, SRC_REAL + 4*idx, tmp);
      }
      for (size_t idx = 0 ; idx < src.size() ; idx += 1) {
	    uint32_t tmp = src[idx].imag() * (1<<dft_frac) + 0.5;
	    dft_write32(bus, SRC_IMAG + 4*idx, tmp);
      }

      printf("Wait 4 clocks...\n");
      fflush(stdout);
      simbus_axi4_wait(bus, 4, 0);

      struct cpair_s { uint32_t real, imag; };
      vector<cpair_s> dst;
      dst.resize(src.size());
      for (size_t idx = 0 ; idx < dst.size() ; idx += 1) {
	    dft_write32(bus, DFT_IDX, idx);
	    dft_write32(bus, DFT_CmdStatus, 0);

	    printf("Wait 4 clocks...\n");
	    fflush(stdout);
	    simbus_axi4_wait(bus, 4, 0);

	    dft_cmd_status = dft_read32(bus, DFT_CmdStatus);
	    printf("DFT CmdStatus = 0x%08" PRIx32 "\n", dft_cmd_status);

	    dst[idx].real = dft_read32(bus, DFT_REAL);
	    dst[idx].imag = dft_read32(bus, DFT_IMAG);
      }

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

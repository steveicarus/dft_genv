
BUILD.v: BUILD.sh DFT_FPGA.v idft_slice.v
	sh BUILD.sh > BUILD.v

idft_slice.v: ../../idft_gen Makefile
	../../idft_gen --name=idft_slice --N=32 > idft_slice.v

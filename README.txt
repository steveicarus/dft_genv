
Here are some DFT test implementations that lead up to a program that
generates a Verilog implementation of a DFT engine. The purpose of
this code is to learn about the Discrete Fourier Transform and its
efficient implemenation. There are a couple C++ programs that
demonstrate aspects of the DFT.

NOTE: This is called a *Discrete* Fourier Transform because it is
performed on a vector of samples, and on on continuous functions. It
is the DFT that computers typically encounter when dealing with actual
data.

* dft_fftw3

This is a test program that uses the fftw3 library to do the work. In
any practical software, there is no reason to not use fftw3, so this
is the reference.

* dft_brute_force

This program is a nieve, brute force implementation of the DFT. It is
slow, but it follows the definition literally and faithfully. This
program shows the math in its raw form.

* dft_brute_force_w

This is still brute force, but demonstrates the point that all the
transcendental functions do not depend on the input data, so can be
reduced to constants that are precalculated and separated from the
actual processing. This is an important insight that makes the FFT
plausible.

* dft_recurse

This program goes all in an demonstrates a divide-and-conquer method
for converting the DFT into O(NlogN) complexity. This is, essentially,
the FFT algorithm laid bare. This code is heavily commented to explain
the process and assumptions tha go into it.

* idft_brute_force

This is a version of dft_brute_force_w, but using fixed point
arithmetic instead of floating point. This is a step towards hardware
implementations that are meant to run in real time. This is brute
force, but shows how accuracies fail when doing integer math, and
helps judge if that's an issue.

* idft_recurse

This is the fixed-point version of dft_recurse.

* idft_gen

This is the goal: a program that generates a Verilog implementation of
the DFT. The input is the size of the DFT engine (The value of N, the
number of samples) and the output is Verilog code that supports DFT of
that size. The generated module is named by a command line option, and
the value if N is also a command line option. (N should be an even
power of 2.) The generated Verilog module generates an output sample
in constant time, and is run N times to generate the N-output vector.

* test_data

This directory contains some test vectors, in csv format.

* test_zynq

Test the idft_gen output in an actual FPGA by running it in a
microzed.


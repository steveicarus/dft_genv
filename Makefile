
CXX = g++
CXXFLAGS = -Wall -O

all: dft_fftw3 \
     dft_brute_force \
     dft_brute_force_w \
     dft_recurse \
     idft_brute_force \
     idft_recurse

dft_fftw3: dft_fftw3.o read_values.o
	$(CXX) -o dft_fftw3 dft_fftw3.o read_values.o -lfftw3

dft_brute_force: dft_brute_force.o read_values.o
	$(CXX) -o dft_brute_force dft_brute_force.o read_values.o

dft_brute_force_w: dft_brute_force_w.o read_values.o
	$(CXX) -o dft_brute_force_w dft_brute_force_w.o read_values.o

dft_recurse: dft_recurse.o read_values.o
	$(CXX) -o dft_recurse dft_recurse.o read_values.o

idft_brute_force: idft_brute_force.o read_values.o
	$(CXX) -o idft_brute_force idft_brute_force.o read_values.o

idft_recurse: idft_recurse.o read_values.o
	$(CXX) -o idft_recurse idft_recurse.o read_values.o

dft_fftw3.o: dft_fftw3.cc priv.h
dft_brute_force.o: dft_brute_force.cc priv.h
dft_brute_force_w.o: dft_brute_force_w.cc priv.h
dft_recurse.o: dft_recurse.cc priv.h
idft_brute_force.o: idft_brute_force.cc icomplex.h priv.h
idft_recurse.o: idft_recurse.cc icomplex.h priv.h
read_values.o: read_values.cc priv.h

idft_gen: idft_gen.cc
	g++ -Wall -o idft_gen idft_gen.cc

comp_test.out: comp_test.v idft_comp.v
	/usr/local/test/bin/iverilog -g2012 -o comp_test.out comp_test.v idft_comp.v

idft_comp.v: idft_gen
	./idft_gen --N=4 > idft_comp.v

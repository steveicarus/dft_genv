#ifndef __icomplex_H
#define __icomplex_H
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
 * The icomplex<> template implements fixed precision (integer)
 * complex numbers. The template is parameterized by the base integer
 * type to use (i.e. int16_t, int32_t, int64_t) and the number of
 * bits of the base integer type to use as fraction bits.
 */

# include  <complex>
# include  <cassert>
# include  <cmath>

template <class TYPE, unsigned FRAC> class icomplex {

    public: // Various constructors.
      inline icomplex()
      : real_(0),  imag_(0)
      { }

      inline icomplex(const icomplex<TYPE,FRAC>&that)
      : real_(that.real_), imag_(that.imag_)
      { }

      inline icomplex(TYPE rv, TYPE iv)
      : real_(rv), imag_(iv)
      { }

      inline icomplex(double rv, double iv = 0.0)
      : real_(rv * (1<<FRAC) + 0.5), imag_(iv * (1<<FRAC) + 0.5)
      { }

      inline icomplex(const std::complex<double>&that)
      : real_(that.real() * (1<<FRAC) + 0.5), imag_(that.imag() * (1<<FRAC) + 0.5)
      { }

    public: // Assignment operators.
      inline icomplex<TYPE,FRAC>& operator =  (const icomplex<TYPE,FRAC>&that)
      {
	    real_ = that.real_;
	    imag_ = that.imag_;
	    return *this;
      }

      inline icomplex<TYPE,FRAC>& operator += (const icomplex<TYPE,FRAC>&that)
      {
	    real_ += that.real_;
	    imag_ += that.imag_;
	    return *this;
      }

    public:
	// These methods return the raw bits of the real and imaginary
	// components. It is up to the user to interpret the fraction bits.
      inline TYPE real(void) const { return real_; }
      inline TYPE imag(void) const { return imag_; }

	// These methods access the real and imaginary parts as
	// doubles. This properly interprets the fractional part.
      inline double d_real(void) const { return (double)real_ / (1 << FRAC); }
      inline double d_imag(void) const { return (double)imag_ / (1 << FRAC); }
      
    private:
      TYPE real_;
      TYPE imag_;
};

template <class TYPE, unsigned FRAC>
inline class icomplex<TYPE,FRAC> operator + (icomplex<TYPE,FRAC>a, icomplex<TYPE,FRAC>b)
{
      return icomplex<TYPE,FRAC> (a.real()+b.real(), a.imag()+b.imag());
}

template <class TYPE, unsigned FRAC>
inline class icomplex<TYPE,FRAC> operator * (icomplex<TYPE,FRAC>a, icomplex<TYPE,FRAC>b)
{
      int64_t rtmp;
      rtmp  = (a.real() * b.real());
      rtmp -= (a.imag() * b.imag());
      rtmp /= 1<<FRAC;
      assert(log2(abs(rtmp)) < (8*sizeof(TYPE)));

      int64_t itmp;
      itmp  = (a.real() * b.imag());
      itmp += (a.imag() * b.real());
      itmp /= 1<<FRAC;
      assert(log2(abs(itmp)) < (8*sizeof(TYPE)));

      return icomplex<TYPE,FRAC> ((TYPE)rtmp, (TYPE)itmp);
}


#endif

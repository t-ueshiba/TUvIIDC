/*
 *  平成9-19年（独）産業技術総合研究所 著作権所有
 *  
 *  創作者：植芝俊夫
 *
 *  本プログラムは（独）産業技術総合研究所の職員である植芝俊夫が創作し，
 *  （独）産業技術総合研究所が著作権を所有する秘密情報です．創作者によ
 *  る許可なしに本プログラムを使用，複製，改変，第三者へ開示する等の著
 *  作権を侵害する行為を禁止します．
 *  
 *  このプログラムによって生じるいかなる損害に対しても，著作権所有者お
 *  よび創作者は責任を負いません。
 *
 *  Copyright 1997-2007.
 *  National Institute of Advanced Industrial Science and Technology (AIST)
 *
 *  Creator: Toshio UESHIBA
 *
 *  [AIST Confidential and all rights reserved.]
 *  This program is confidential. Any using, copying, changing or
 *  giving any information concerning with this program to others
 *  without permission by the creator are strictly prohibited.
 *
 *  [No Warranty.]
 *  The copyright holders or the creator are not responsible for any
 *  damages in the use of this program.
 *  
 *  $Id: GaussianConvolver++.h,v 1.1 2008-08-11 07:09:34 ueshiba Exp $
 */
#ifndef	__TUGaussianConvolver_h
#define	__TUGaussianConvolver_h

#include "TU/Vector++.h"
#include "TU/IIRFilter++.h"

namespace TU
{
/************************************************************************
*  class GaussianCoefficients						*
************************************************************************/
class GaussianCoefficients
{
  private:
    struct Params
    {
	void		set(double aa, double bb, double tt, double aaa);
	Params&		operator -=(const Vector<double>& p)		;
    
	double		a, b, theta, alpha;
    };

    class EvenConstraint
    {
      public:
	typedef double		ET;
	typedef Array<Params>	AT;

	EvenConstraint(ET sigma) :_sigma(sigma)				{}
	
	Vector<ET>	operator ()(const AT& params)		const	;
	Matrix<ET>	jacobian(const AT& params)		const	;

      private:
	ET		_sigma;
    };

    class CostFunction
    {
      public:
	typedef double		ET;
	typedef Array<Params>	AT;
    
	enum			{D = 2};

	CostFunction(int ndivisions, ET range)
	    :_ndivisions(ndivisions), _range(range)			{}
    
	Vector<ET>	operator ()(const AT& params)		 const	;
	Matrix<ET>	jacobian(const AT& params)		 const	;
	void		update(AT& params, const Vector<ET>& dp) const	;

      private:
	const int	_ndivisions;
	const ET	_range;
    };

  public:
    void	initialize(float sigma)			;
    
  protected:
    GaussianCoefficients(float sigma)			{initialize(sigma);}
    
  protected:
    float	_c0[8];		// forward coefficients for smoothing
    float	_c1[8];		// forward coefficients for 1st derivatives
    float	_c2[8];		// forward coefficients for 2nd derivatives
};
    
/************************************************************************
*  class GaussianConvoler						*
************************************************************************/
//! Gauss核による1次元配列畳み込みを行うクラス
class GaussianConvolver
    : public GaussianCoefficients, private BilateralIIRFilter<4u>
{
  public:
    typedef BilateralIIRFilter<4u>		BIIRF;

    GaussianConvolver(float sigma=1.0)	:GaussianCoefficients(sigma)	{}

    template <class T1, class B1, class T2, class B2> GaussianConvolver&
	smooth(const Array<T1, B1>& in, Array<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver&
	diff(const Array<T1, B1>& in, Array<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver&
	diff2(const Array<T1, B1>& in, Array<T2, B2>& out)		;
};

//! Gauss核によるスムーシング
/*!
  \param in	入力1次元配列
  \param out	出力1次元配列
  \return	このGauss核自身
*/
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver&
GaussianConvolver::smooth(const Array<T1, B1>& in, Array<T2, B2>& out)
{
    BIIRF::initialize(_c0, BIIRF::Zeroth).convolve(in, out);

    return *this;
}

//! Gauss核による1階微分
/*!
  \param in	入力1次元配列
  \param out	出力1次元配列
  \return	このGauss核自身
*/
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver&
GaussianConvolver::diff(const Array<T1, B1>& in, Array<T2, B2>& out)
{
    BIIRF::initialize(_c1, BIIRF::First).convolve(in, out);

    return *this;
}

//! Gauss核による2階微分
/*!
  \param in	入力1次元配列
  \param out	出力1次元配列
  \return	このGauss核自身
*/
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver&
GaussianConvolver::diff2(const Array<T1, B1>& in, Array<T2, B2>& out)
{
    BIIRF::initialize(_c2, BIIRF::Second).convolve(in, out);

    return *this;
}

/************************************************************************
*  class GaussianConvoler2<BIIRH, BIIRV>				*
************************************************************************/
//! Gauss核による2次元配列畳み込みを行うクラス
template <class BIIRH=BilateralIIRFilter<4u>, class BIIRV=BIIRH>
class GaussianConvolver2
    : public GaussianCoefficients, private BilateralIIRFilter2<BIIRH, BIIRV>
{
  public:
    typedef BilateralIIRFilter<4u>		BIIRF;
    typedef BilateralIIRFilter2<BIIRH, BIIRV>	BIIRF2;

    GaussianConvolver2(float sigma=1.0)	:GaussianCoefficients(sigma) 	{}
    GaussianConvolver2(float sigma, u_int nthreads)
    	:GaussianCoefficients(sigma), BIIRF2(nthreads) 			{}

    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	smooth(const Array2<T1, B1>& in, Array2<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	diffH(const Array2<T1, B1>& in, Array2<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	diffV(const Array2<T1, B1>& in, Array2<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	diffHH(const Array2<T1, B1>& in, Array2<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	diffHV(const Array2<T1, B1>& in, Array2<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	diffVV(const Array2<T1, B1>& in, Array2<T2, B2>& out)		;
    template <class T1, class B1, class T2, class B2> GaussianConvolver2&
	laplacian(const Array2<T1, B1>& in, Array2<T2, B2>& out)	;

  private:
    Array2<Array<float> >	_tmp;	// buffer for computing Laplacian
};

//! Gauss核によるスムーシング
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::smooth(const Array2<T1, B1>& in,
					 Array2<T2, B2>& out)
{
    BIIRF2::initialize(_c0, BIIRF::Zeroth,
		       _c0, BIIRF::Zeroth).convolve(in, out);

    return *this;
}

//! Gauss核による横方向1階微分(DOG)
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::diffH(const Array2<T1, B1>& in,
					Array2<T2, B2>& out)
{
    BIIRF2::initialize(_c1, BIIRF::First,
		       _c0, BIIRF::Zeroth).convolve(in, out);

    return *this;
}

//! Gauss核による縦方向1階微分(DOG)
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::diffV(const Array2<T1, B1>& in,
					Array2<T2, B2>& out)
{
    BIIRF2::initialize(_c0, BIIRF::Zeroth,
		       _c1, BIIRF::First).convolve(in, out);

    return *this;
}

//! Gauss核による横方向2階微分
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::diffHH(const Array2<T1, B1>& in,
					 Array2<T2, B2>& out)
{
    BIIRF2::initialize(_c2, BIIRF::Second,
		       _c0, BIIRF::Zeroth).convolve(in, out);

    return *this;
}

//! Gauss核による縦横両方向2階微分
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::diffHV(const Array2<T1, B1>& in,
					 Array2<T2, B2>& out)
{
    BIIRF2::initialize(_c1, BIIRF::First,
		       _c1, BIIRF::First).convolve(in, out);

    return *this;
}

//! Gauss核による縦方向2階微分
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::diffVV(const Array2<T1, B1>& in,
					 Array2<T2, B2>& out)
{
    BIIRF2::initialize(_c0, BIIRF::Zeroth,
		       _c2, BIIRF::Second).convolve(in, out);

    return *this;
}

//! Gauss核によるラプラシアン(LOG)
/*!
  \param in	入力2次元配列
  \param out	出力2次元配列
  \return	このGauss核自身
*/
template <class BIIRH, class BIIRV>
template <class T1, class B1, class T2, class B2>
inline GaussianConvolver2<BIIRH, BIIRV>&
GaussianConvolver2<BIIRH, BIIRV>::laplacian(const Array2<T1, B1>& in,
					    Array2<T2, B2>& out)
{
    diffHH(in, _tmp).diffVV(in, out);
    out += _tmp;
    
    return *this;
}

}

#endif	/* !__TUGaussianConvolver_h */

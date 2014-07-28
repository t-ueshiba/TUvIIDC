/*
 *  平成21-22年（独）産業技術総合研究所 著作権所有
 *  
 *  創作者：植芝俊夫
 *
 *  本プログラムは（独）産業技術総合研究所の職員である植芝俊夫が創作し，
 *  （独）産業技術総合研究所が著作権を所有する秘密情報です．著作権所有
 *  者による許可なしに本プログラムを使用，複製，改変，第三者へ開示する
 *  等の行為を禁止します．
 *  
 *  このプログラムによって生じるいかなる損害に対しても，著作権所有者お
 *  よび創作者は責任を負いません。
 *
 *  Copyright 2009-2010.
 *  National Institute of Advanced Industrial Science and Technology (AIST)
 *
 *  Creator: Toshio UESHIBA
 *
 *  [AIST Confidential and all rights reserved.]
 *  This program is confidential. Any using, copying, changing or
 *  giving any information concerning with this program to others
 *  without permission by the copyright holder are strictly prohibited.
 *
 *  [No Warranty.]
 *  The copyright holder or the creator are not responsible for any
 *  damages caused by using this program.
 *
 *  $Id: SURFCreator.h,v 1.16 2012-01-24 02:44:54 ueshiba Exp $
 */
#ifndef __TU_SURFCREATOR_H
#define __TU_SURFCREATOR_H

#include <boost/iterator/iterator_adaptor.hpp>
#include "TU/Feature.h"
#include "TU/IntegralImage.h"
#include "TU/Heap.h"
#if defined(USE_TBB)
#  include <tbb/parallel_for.h>
#  include <tbb/blocked_range.h>
#  include <tbb/spin_mutex.h>
#endif

namespace TU
{
typedef Feature<int, 128u>	SIFT;
typedef Feature<float, 64u>	SURF;
typedef Feature<float, 128u>	SURF128;

/************************************************************************
*  class SURFCreator							*
************************************************************************/
//! SURF特徴点を検出してその記述子を作るクラス
class SURFCreator
{
  public:
  //! 数値計算全般のベースとなる型
    typedef float	value_type;

  //! 各種パラメータ
    struct Parameters
    {
	Parameters()							;

	size_t		nScales;
	size_t		nOctaves;
	value_type	scoreThresh;
	size_t		initialBoxFilterSize;
	size_t		scaleOverlap;
    };

  //! 検出された特徴点の挿入子
    template <class F> class Insertor
    {
      public:	
	virtual void	operator ()(const F& feature)			= 0;
    };
    
  private:
    class BoxFilter
    {
      public:
	BoxFilter(const IntegralImage<value_type>& image,
		  value_type baseSize)					;

	BoxFilter&	setBaseSize(value_type baseSize)		;
	BoxFilter&	setY(size_t y)					;

	value_type	getDxx(size_t x)			const	;
	value_type	getDyy(size_t x)			const	;
	value_type	getDxy(size_t x)			const	;
	value_type	getDet(size_t x)			const	;
#if defined(SSE)
	template <size_t O>
	mm::F32vec	mmCrop2(int umin, int umax,
				int vmin, int vmax)		const	;
	template <size_t O>
	mm::F32vec	mmGetDxx(size_t x)			const	;
	template <size_t O>
	mm::F32vec	mmGetDyy(size_t x)			const	;
	template <size_t O>
	mm::F32vec	mmGetDxy(size_t x)			const	;
	template <size_t O>
	mm::F32vec	mmGetDet(size_t x)			const	;
#endif
	value_type	getWx(size_t x, size_t y);
	value_type	getWy(size_t x, size_t y);

	bool		checkBounds(int x, int y)		const	;
	bool		checkBoundsW(int x, int y)		const	;

      private:
	const IntegralImage<value_type>&
			_integralImage;
	value_type	_sqCorrectFactor;	// precomp values for det

	int		_lc;		//!< Dxy の4つのboxの幅
	int		_lm;		//!< Dxx, Dyy の中央のboxの半幅
	int		_lr;		//!< Dxx, Dyy の外接boxの長辺の半幅
	int		_lb;		//!< Dxx, Dyy の外接boxの短辺の半幅
	int		_w1;		//!< Wx, Wy の2つのboxの短辺の幅
	
	size_t		_y_lb_min;
	size_t		_y_lb_max;
	size_t		_y_lr_min;
	size_t		_y_lr_max;
	size_t		_y_lm_min;
	size_t		_y_lm_max;
	size_t		_y_lc_min;
	size_t		_y_lc_max;
	size_t		_y;
    };

    enum		{NSubRegions = 4, VecLenMax = 8};

    typedef value_type	HistBin[VecLenMax];
    typedef HistBin	HistSlice[NSubRegions+2];
    typedef HistSlice	Histogram[NSubRegions+2];
#if defined(USE_TBB)
    template <class I>
    class Apply
    {
      private:
	typedef typename std::iterator_traits<I>::value_type		F;

      public:
	Apply(const SURFCreator& creator,
	      void (SURFCreator::*func)(F&) const)
	    :_creator(creator), _func(func)				{}
	
	void		operator ()(const tbb::blocked_range<I>& r) const
			{
			    for (I iter = r.begin(); iter != r.end(); ++iter)
				(_creator.*_func)(*iter);
			}

      private:
	const SURFCreator&	_creator;
	void			(SURFCreator::*_func)(F&) const;
    };

    class CalcDetsLine
    {
      public:
	CalcDetsLine(const SURFCreator& creator, size_t o,
		     size_t filterSize, size_t borderSize,
		     Matrix<value_type>& det)
	    :_creator(creator), _o(o), _filterSize(filterSize),
	     _borderSize(borderSize), _det(det)				{}
	
	void		operator ()(const tbb::blocked_range<size_t>& r) const
			{
			    for (size_t y = r.begin(); y != r.end(); ++y)
				_creator.calcDetsLine(y, _o, _filterSize,
						      _borderSize, _det);
			}

      private:
	const SURFCreator&	_creator;
	const size_t		_o;
	const size_t		_filterSize;
	const size_t		_borderSize;
	Matrix<value_type>&	_det;
    };

    template <class F>
    class DetectLine
    {
      public:
	DetectLine(const SURFCreator& creator, size_t s, size_t pixelStep,
		   const Array<Matrix<value_type> >& det,
		   const Array<size_t>& borderSizes,
		   Insertor<F>& insert)
	    :_creator(creator), _s(s), _pixelStep(pixelStep),
	     _det(det), _borderSizes(borderSizes), _insert(insert)	{}
	
	void		operator ()(const tbb::blocked_range<size_t>& r) const
			{
			    const size_t	y0 = _borderSizes[_s+1] + 1;
			    for (size_t y2 = r.begin(); y2 != r.end(); ++y2)
				_creator.detectLine(y0 + 2*y2, _s, _pixelStep,
						    _det, _borderSizes,
						    _insert);
			}

      private:
	const SURFCreator&			_creator;
	const size_t				_s;
	const size_t				_pixelStep;
	const Array<Matrix<value_type> >&	_det;
	const Array<size_t>&			_borderSizes;
	Insertor<F>&				_insert;
    };
#endif

  public:
    SURFCreator()							{}
    SURFCreator(const Parameters& params)	:_params(params)	{}

    SURFCreator&	setParameters(const Parameters& parameters)	;
    const Parameters&	getParameters()				  const	;
    
    template <class T, class F>
    SURFCreator&	detectFeatures(const Image<T>& image,
				       Insertor<F>& insert)		;
    template <class ITER>
    SURFCreator&	makeDescriptors(ITER begin, ITER end)	;
    
  private:
    template <class T, size_t D>
    void		assignOrientation(Feature<T, D>& feature) const	;
    template <class T, size_t D>
    void		makeDescriptor(Feature<T, D>& feature)	  const	;
    void		calcDets(size_t octave, size_t scale,
				 Matrix<value_type>& det,
				 size_t& borderSize)		  const	;
    void		calcDetsLine(size_t y, size_t octave,
				     size_t filterSize,
				     size_t borderSize,
				     Matrix<value_type>& det)	  const	;
    template <class F>
    void		detect(size_t octave, size_t scale,
			       const Array<Matrix<value_type> >& det,
			       const Array<size_t>& borderSizes,
			       Insertor<F>& insert)		  const	;
    template <class F>
    void		detectLine(size_t y, size_t s, size_t pixelStep,
				   const Array<Matrix<value_type> >& det,
				   const Array<size_t>& borderSizes,
				   Insertor<F>& insert)		  const	;
    static bool		fineTuneExtrema(
			    const Array<Matrix<value_type> >& det,
			    int x, int y, int s, 
			    value_type& xf, value_type& yf,
			    value_type& sf, value_type& score,
			    size_t octaveWidth,
			    size_t octaveHeight,
			    size_t borderSize)				;
    bool		calcTrace(value_type x, value_type y,
				  value_type scale, int& trace)	  const	;
    size_t		getFilterSize(size_t octave, size_t scale) const;
    size_t		getBorderSize(size_t octave, size_t scale) const;
    template <size_t D>
    static void		vote(value_type uIdx, value_type vIdx,
			     value_type wu,   value_type wv,
			     Histogram bins)				;
    
  private:
    IntegralImage<value_type>		_integralImage;
    Parameters				_params;

    static const value_type		_magFactor;
    static const value_type		_baseSigma;
};

inline SURFCreator&
SURFCreator::setParameters(const Parameters& params)
{
    _params = params;
    return *this;
}

inline const SURFCreator::Parameters&
SURFCreator::getParameters() const
{
    return _params;
}

template <class T, class F> SURFCreator&
SURFCreator::detectFeatures(const Image<T>& image, Insertor<F>& insert)
{
    _integralImage.initialize(image);	// 積分画像を作る
    
    Array<size_t>		borderSizes(_params.nScales);
    Array<Matrix<value_type> >	det(_params.nScales);
    for (size_t s = 0; s < det.size(); ++s)
	det[s].resize(_integralImage.originalHeight(),
		      _integralImage.originalWidth());

  // baseSize + 3 times first increment for step back
  // for the first octave 9x9, 15x15, 21x21, 27x27, 33x33 
  // for the second 21x21, 33x33, 45x45 ...
    for (size_t o = 0; o < _params.nOctaves; ++o)     // 全オクターブについて...
    {
	for (size_t s = 0; s < _params.nScales; ++s)  // 全スケールについて...
	    calcDets(o, s, det[s], borderSizes[s]);

	for (size_t s = 1; s < (_params.nScales - 1); s += 2)
	    detect(o, s, det, borderSizes, insert);
    }

    return *this;
}

template <class ITER> SURFCreator&
SURFCreator::makeDescriptors(ITER begin, ITER end)
{
    typedef typename std::iterator_traits<ITER>::value_type
							feature_type;
    typedef typename feature_type::value_type		feature_value_type;

#if defined(USE_TBB)
    using namespace	tbb;
    
    parallel_for(blocked_range<ITER>(begin, end, 1),
		 Apply<ITER>(
		     *this,
		     &SURFCreator::assignOrientation<
			 feature_value_type, feature_type::DescriptorDim>));
    parallel_for(blocked_range<ITER>(begin, end, 1),
		 Apply<ITER>(
		     *this,
		     &SURFCreator::makeDescriptor<
			 feature_value_type, feature_type::DescriptorDim>));
#else
    for (ITER feature = begin; feature != end; ++feature)
    {
	assignOrientation(*feature);
	makeDescriptor(*feature);
    }
#endif
    return *this;
}
    
inline void
SURFCreator::calcDets(size_t o, size_t s,
		      Matrix<value_type>& det, size_t& borderSize) const
{
    borderSize = getBorderSize(o, s);

    const size_t	pixelStep  = 1 << o;			// 2^octave
    const size_t	filterSize = getFilterSize(o, s);
    const size_t	yend	   = _integralImage.originalHeight()
				   / pixelStep - borderSize;
#if defined(USE_TBB)
    tbb::parallel_for(tbb::blocked_range<size_t>(borderSize, yend, 1),
		      CalcDetsLine(*this, o, filterSize, borderSize, det));
#else
    for (size_t y = borderSize; y < yend; ++y)
	calcDetsLine(y, o, filterSize, borderSize, det);
#endif
}

template <class F> void
SURFCreator::detect(size_t o, size_t s,
		    const Array<Matrix<value_type> >& det,
		    const Array<size_t>& borderSizes,
		    Insertor<F>& insert) const
{
    const size_t	pixelStep  = 1 << o;			// 2^octave
    const size_t	borderSize = borderSizes[s+1];
    const size_t	yend	   = _integralImage.originalHeight()
				   / pixelStep - borderSize - 1;
#if defined(USE_TBB)
    tbb::parallel_for(tbb::blocked_range<size_t>(
			  0, (yend - borderSize - 1)/2, 1),
		      DetectLine<F>(
			  *this, s, pixelStep, det, borderSizes, insert));
#else
    for (size_t y = borderSize + 1; y < yend; y += 2)
	detectLine(y, s, pixelStep, det, borderSizes, insert);
#endif
}

/************************************************************************
*  class Sieve<F, CMP>							*
************************************************************************/
template <class F, class CMP=std::greater<F> >
class Sieve : public container<Sieve<F, CMP> >
{
  private:
    typedef Array<Heap<F, CMP> >	HeapArray;
    typedef Array2<HeapArray>		HeapArray2;
    
  public:
    class Insertor : public SURFCreator::Insertor<F>
    {
      public:
	Insertor(Sieve& sieve)	:_sieve(sieve)	{}
	virtual void	operator ()(const F& feature)
			{
#if defined(USE_TBB)
			    tbb::spin_mutex::scoped_lock	lock(_mutex);
#endif
			    _sieve.insert(feature);
			}
	
      private:
	Sieve&		_sieve;
#if defined(USE_TBB)
	tbb::spin_mutex	_mutex;
#endif
    };

    template <class SIEVE, class BASE>
    class Iterator
	: public boost::iterator_adaptor<Iterator<SIEVE, BASE>,
					 BASE,
					 boost::use_default,
					 std::bidirectional_iterator_tag>
    {
      private:
	typedef boost::iterator_adaptor<
		    Iterator,
		    BASE,
		    boost::use_default,
		    std::bidirectional_iterator_tag>	super;

      public:
	typedef typename super::difference_type		difference_type;
	typedef typename super::value_type		value_type;
	typedef typename super::pointer			pointer;
	typedef typename super::reference		reference;
	typedef typename super::iterator_category	iterator_category;
	
	friend class	boost::iterator_core_access;

      public:
	Iterator(SIEVE sieve, bool isBegin)
	    :super(isBegin ? sieve._buckets[0][0].begin() :
		   sieve._buckets[sieve._buckets.nrow()-1]
		    		 [sieve._buckets.ncol()-1].end()),
	     _sieve(sieve),
	     _i(isBegin ? 0 : _sieve._buckets.nrow() - 1),
	     _j(isBegin ? 0 : _sieve._buckets.ncol() - 1)		{}

      private:
	void	increment()
		{
		    if (++super::base_reference() ==
			_sieve._buckets[_i][_j].end())
		    {
			if (++_j == _sieve._buckets.ncol())
			{
			    if (++_i == _sieve._buckets.nrow())
				return;
			    _j = 0;
			}
			super::base_reference()
			    = _sieve._buckets[_i][_j].begin();
		    }
		}
	void	decrement()
		{
		    if (super::base() == _sieve._buckets[_i][_j].begin())
		    {
			if (_j == 0)
			{
			    if (_i == 0)
				return;
			    --_i;
			    _j = _sieve._buckets.ncol();
			}
			super::base_reference()
			    = _sieve._buckets[_i][--_j].end();
		    }
		    --super::base_reference();
		}
	
      private:
	SIEVE	_sieve;
	size_t	_i, _j;
    };

    typedef F							value_type;
    typedef Iterator<const Sieve&,
		     typename Heap<F, CMP>::const_iterator>	const_iterator;
    typedef Iterator<Sieve&, typename Heap<F, CMP>::iterator>	iterator;
    
  public:
    Sieve(size_t height, size_t width,
	  size_t r, size_t c, size_t max_siz)	;

    size_t		nrow()		const	{ return _buckets.nrow(); }
    size_t		ncol()		const	{ return _buckets.ncol(); }
    size_t		max_size()	const	{ return _max_size; }
    size_t		size()		const	;
    bool		insert(const F& feature);
    void		clear()			;
    iterator		begin()
			{
			    return iterator(*this, true);
			}
    const_iterator	begin() const
			{
			    return const_iterator(*this, true);
			}
    const_iterator	cbegin() const
			{
			    return begin();
			}
    iterator		end()
			{
			    return iterator(*this, false);
			}
    const_iterator	end() const
			{
			    return const_iterator(*this, false);
			}
    const_iterator	cend() const
			{
			    return end();
			}
	
  private:
    const size_t	_max_size;
    HeapArray2		_buckets;
    const float		_dx, _dy;
};

template <class F, class CMP>
Sieve<F, CMP>::Sieve(size_t height, size_t width,
		     size_t r, size_t c, size_t max_siz)
    :_max_size(max_siz), _buckets(r, c),
     _dy(float(r)/float(height)), _dx(float(c)/float(width))
{
    for (size_t i = 0; i < _buckets.nrow(); ++i)
	for (size_t j = 0; j < _buckets.ncol(); ++j)
	    _buckets[i][j].resize(_max_size);
}

template <class F, class CMP> size_t
Sieve<F, CMP>::size() const
{
    size_t	n = 0;
    for (size_t i = 0; i < _buckets.nrow(); ++i)
	for (size_t j = 0; j < _buckets.ncol(); ++j)
	    n += _buckets[i][j].size();

    return n;
}

template <class F, class CMP> inline bool
Sieve<F, CMP>::insert(const F& feature)
{
    size_t	i = size_t(feature[1]*_dy), j = size_t(feature[0]*_dx);
    if (i >= _buckets.nrow())
	i = _buckets.nrow() - 1;
    if (j >= _buckets.ncol())
	j = _buckets.ncol() - 1;
    return _buckets[i][j].push_or_replace(feature);
}

template <class F, class CMP> void
Sieve<F, CMP>::clear()
{
    for (size_t i = 0; i < _buckets.nrow(); ++i)
	for (size_t j = 0; j < _buckets.ncol(); ++j)
	    _buckets[i][j].clear();
}

/************************************************************************
*  global functions							*
************************************************************************/
//! 画像からSURF特徴を抽出
template <class F> Array<F>
createSURF(const Image<u_char>& image, const SURFCreator::Parameters& params)
{
    typedef typename Sieve<F>::Insertor		Insertor;
    
  // SURF検出器を生成しパラメータを設定する．
    SURFCreator	creator(params);

  // 画像全体を10x10のバケットに区切り，
  // 各バケットにスコアの高い順に高々10個のSURFを登録する．
    Sieve<F>	sieve(image.height(), image.width(), 10, 10, 10);
    Insertor	insertor(sieve);
    creator.detectFeatures(image, insertor);

  // 各SURF特徴点に向きと特徴ベクトルを与える．
    Array<F>	features(sieve);
    creator.makeDescriptors(features.begin(), features.end());

    return features;
}

}
#endif	// !__TU_SURFCREATOR_H
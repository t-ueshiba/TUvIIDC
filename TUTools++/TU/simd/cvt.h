/*
 *  $Id$
 */
#if !defined(__TU_SIMD_CVT_H)
#define	__TU_SIMD_CVT_H

#include "TU/simd/zero.h"

namespace TU
{
namespace simd
{
/************************************************************************
*  Type conversion operators						*
************************************************************************/
//! T型ベクトルのI番目の部分をS型ベクトルに型変換する．
/*!
  整数ベクトル間の変換の場合，SのサイズはTの2/4/8倍である．また，S, Tは
  符号付き／符号なしのいずれでも良いが，符号付き -> 符号なしの変換はできない．
  \param x	変換されるベクトル
  \return	変換されたベクトル
*/
template <class S, size_t I=0, class T> static inline vec<S>
cvt(vec<T> x)
{
    typedef typename type_traits<S>::lower_type	L;
    
    return cvt<S, (I&0x1)>(cvt<L, (I>>1)>(x));
}

//! 2つのT型整数ベクトルをより小さなS型整数ベクトルに型変換する．
/*!
  Tは符号付き整数型，SはTの半分のサイズを持つ符号付き／符号なし整数型
  である．Sが符号付き／符号なしのいずれの場合も飽和処理が行われる．
  \param x	変換されるベクトル
  \param y	変換されるベクトル
  \return	xが変換されたものを下位，yが変換されたものを上位に
		配したベクトル
*/
template <class S, class T> static vec<S>	cvt(vec<T> x, vec<T> y)	;

namespace detail
{
  template <class S, size_t I>
  struct generic_cvt
  {
      vec<S>	operator ()(vec<S> x) const
		{
		    return x;
		}
      template <class T_>
      vec<S>	operator ()(vec<T_> x) const
		{
		    return cvt<S, I>(x);
		}
      template <class T_>
      vec<S>	operator ()(vec<T_> x, vec<T_> y) const
		{
		    return cvt<S>(x, y);
		}
  };
}	// namespace detail
    
template <class S, size_t I=0, class HEAD, class TAIL> static inline auto
cvt(const boost::tuples::cons<HEAD, TAIL>& x)
    -> decltype(boost::tuples::cons_transform(x, detail::generic_cvt<S, I>()))
{
    return boost::tuples::cons_transform(x, detail::generic_cvt<S, I>());
}
    
template <class S, class H1, class T1, class H2, class T2> static inline auto
cvt(const boost::tuples::cons<H1, T1>& x, const boost::tuples::cons<H2, T2>& y)
    -> decltype(boost::tuples::cons_transform(x, y,
					      detail::generic_cvt<S, 0>()))
{
    return boost::tuples::cons_transform(x, y, detail::generic_cvt<S, 0>());
}

}	// namespace simd
}	// namespace TU

#if defined(MMX)
#  include "TU/simd/intel/cvt.h"
#elif defined(NEON)
#  include "TU/simd/arm/cvt.h"
#endif

#endif	// !__TU_SIMD_CVT_H
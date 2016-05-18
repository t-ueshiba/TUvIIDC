/*
 *  $Id$: IIDCCamera.cc 1777 2015-02-06 10:54:01Z ueshiba $
 */
#include "TU/FireWireNode_.h"
#include "TU/USBNode_.h"
#include <libraw1394/csr.h>
#include <algorithm>
#include <cstring>

#define XY_YZ(X, Y, Z)						\
{								\
    rgb->X = *buf;						\
    rgb->Y = (u_int(*(buf+1)) + u_int(*nxt)) >> 1;		\
    rgb->Z = *(nxt+1);						\
    ++rgb;							\
    ++buf;							\
    ++nxt;							\
}

#define YX_ZY(X, Y, Z)						\
{								\
    rgb->X = *(buf+1);						\
    rgb->Y = (u_int(*buf) + u_int(*(nxt+1))) >> 1;		\
    rgb->Z = *nxt;						\
    ++rgb;							\
    ++buf;							\
    ++nxt;							\
}

#define XYX_YZY_XYX(X, Y, Z)					\
{								\
    rgb->X = (u_int(*(prv-1)) + u_int(*(prv+1)) +		\
	       u_int(*(nxt-1)) + u_int(*(nxt+1))) >> 2;		\
    rgb->Y = (u_int(*(buf-1)) +u_int(*(buf+1)) +		\
	       u_int(*prv) + u_int(*nxt)) >> 2;			\
    rgb->Z = *buf;						\
    ++rgb;							\
    ++prv;							\
    ++buf;							\
    ++nxt;							\
}

#define yXy_ZYZ_yXy(X, Y, Z)					\
{								\
    rgb->X = (u_int(*prv) + u_int(*nxt)) >> 1;			\
    rgb->Y = *buf;						\
    rgb->Z = (u_int(*(buf-1)) + u_int(*(buf+1))) >> 1;		\
    ++rgb;							\
    ++prv;							\
    ++buf;							\
    ++nxt;							\
}

namespace TU
{
/************************************************************************
*  static functions							*
************************************************************************/
template <class S, class T> static const S*
bayerRGGB2x2(const S* buf, T* rgb, int w)
{
    const S*	nxt = buf + w;			// next line
    while ((w -= 2) > 0)
    {
	XY_YZ(r, g, b)
	YX_ZY(r, g, b)
    }
    XY_YZ(r, g, b)
    --buf;
    --nxt;
    XY_YZ(r, g, b)
    
    return buf + 1;
}

template <class S, class T> static const S*
bayerRGGBOdd3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    YX_ZY(b, g, r)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 奇数行中間の列を処理
    {
	XYX_YZY_XYX(r, g, b)
	yXy_ZYZ_yXy(r, g, b)
    }
    --buf;
    --nxt;
    YX_ZY(b, g, r)				// 右端の画素は2x2で処理
    
    return buf + 1;
}

template <class S, class T> static const S*
bayerRGGBEven3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    XY_YZ(r, g, b)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 偶数行中間の列を処理
    {
	yXy_ZYZ_yXy(b, g, r)
	XYX_YZY_XYX(b, g, r)
    }
    --buf;
    --nxt;
    XY_YZ(r, g, b)				// 右端の画素は2x2で処理

    return buf + 1;
}

template <class S, class T> static const S*
bayerBGGR2x2(const S* buf, T* rgb, int w)
{
    const S*	nxt = buf + w;			// next line
    while ((w -= 2) > 0)
    {
	XY_YZ(b, g, r)
	YX_ZY(b, g, r)
    }
    XY_YZ(b, g, r)
    --buf;
    --nxt;
    XY_YZ(b, g, r)

    return buf + 1;
}

template <class S, class T> static const S*
bayerBGGROdd3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    YX_ZY(r, g, b)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 奇数行中間の列を処理
    {
	XYX_YZY_XYX(b, g, r)
	yXy_ZYZ_yXy(b, g, r)
    }
    --buf;
    --nxt;
    YX_ZY(r, g, b)				// 右端の画素は2x2で処理

    return buf + 1;
}

template <class S, class T> static const S*
bayerBGGREven3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    XY_YZ(b, g, r)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 偶数行中間の列を処理
    {
	yXy_ZYZ_yXy(r, g, b)
	XYX_YZY_XYX(r, g, b)
    }
    --buf;
    --nxt;
    XY_YZ(b, g, r)				// 右端の画素は2x2で処理

    return buf + 1;
}

template <class S, class T> static const S*
bayerGRBG2x2(const S* buf, T* rgb, int w)
{
    const S*	nxt = buf + w;			// next line
    while ((w -= 2) > 0)
    {
	YX_ZY(r, g, b)
	XY_YZ(r, g, b)
    }
    YX_ZY(r, g, b)
    --buf;
    --nxt;
    YX_ZY(r, g, b)

    return buf + 1;
}

template <class S, class T> static const S*
bayerGRBGOdd3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    XY_YZ(b, g, r)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 奇数行中間の列を処理
    {
	yXy_ZYZ_yXy(r, g, b)
	XYX_YZY_XYX(r, g, b)
    }
    --buf;
    --nxt;
    XY_YZ(b, g, r)				// 右端の画素は2x2で処理

    return buf + 1;
}

template <class S, class T> static const S*
bayerGRBGEven3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    YX_ZY(r, g, b)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 偶数行中間の列を処理
    {
	XYX_YZY_XYX(b, g, r)
	yXy_ZYZ_yXy(b, g, r)
    }
    --buf;
    --nxt;
    YX_ZY(r, g, b)				// 右端の画素は2x2で処理

    return buf + 1;
}

template <class S, class T> static const S*
bayerGBRG2x2(const S* buf, T* rgb, int w)
{
    const S*	nxt = buf + w;			// next line
    while ((w -= 2) > 0)
    {
	YX_ZY(b, g, r)
	XY_YZ(b, g, r)
    }
    YX_ZY(b, g, r)
    --buf;
    --nxt;
    YX_ZY(b, g, r)

    return buf + 1;
}

template <class S, class T> static const S*
bayerGBRGOdd3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    XY_YZ(r, g, b)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 奇数行中間の列を処理
    {
	yXy_ZYZ_yXy(b, g, r)
	XYX_YZY_XYX(b, g, r)
    }
    --buf;
    --nxt;
    XY_YZ(r, g, b)				// 右端の画素は2x2で処理

    return buf + 1;
}

template <class S, class T> static const S*
bayerGBRGEven3x3(const S* buf, T* rgb, int w)
{
    const S	*nxt = buf + w;			// next line
    YX_ZY(b, g, r)				// 左端の画素は2x2で処理
    const S	*prv = buf - w;			// previous line
    while ((w -= 2) > 0)			// 偶数行中間の列を処理
    {
	XYX_YZY_XYX(r, g, b)
	yXy_ZYZ_yXy(r, g, b)
    }
    --buf;
    --nxt;
    YX_ZY(b, g, r)				// 右端の画素は2x2で処理

    return buf + 1;
}

static quadlet_t
triggerModeValue(IIDCCamera::TriggerMode mode)
{
    quadlet_t	val = 0;
    for (u_int n = mode; n >>= 1; )
	++val;

    return 15 - val;
}

static inline u_int
triggerModeInq(quadlet_t val)
{
    return 0x1u << (15 - val);
}

static IIDCNode*
getNode(IIDCCamera::Type type, uint64_t uniqId, u_int delay)
{
    try
    {
	return new FireWireNode(type, uniqId, delay);
    }
    catch(const std::exception& err)
    {
	return new USBNode(type, uniqId);
    }

    return nullptr;
}
    
/************************************************************************
*  local constants							*
************************************************************************/
static const uint32_t	Cur_V_Frm_Rate		= 0x600;
static const uint32_t	Cur_V_Mode		= 0x604;
static const uint32_t	Cur_V_Format		= 0x608;
static const uint32_t	ISO_Channel		= 0x60c;
static const uint32_t	Camera_Power		= 0x610;
static const uint32_t	ISO_EN			= 0x614;
static const uint32_t	Memory_Save		= 0x618;
static const uint32_t	One_Shot		= 0x61c;
static const uint32_t	Mem_Save_Ch		= 0x620;
static const uint32_t	Cur_Mem_Ch		= 0x624;

static const quadlet_t	One_Push		= 0x1u << 26;
static const quadlet_t	ON_OFF			= 0x1u << 25;
static const quadlet_t	A_M_Mode		= 0x1u << 24;

// Video Mode CSR for Format_7.
static const uint32_t	MAX_IMAGE_SIZE_INQ	= 0x000;
static const uint32_t	UNIT_SIZE_INQ		= 0x004;
static const uint32_t	IMAGE_POSITION		= 0x008;
static const uint32_t	IMAGE_SIZE		= 0x00c;
static const uint32_t	COLOR_CODING_ID		= 0x010;
static const uint32_t	COLOR_CODING_INQ	= 0x014;
static const uint32_t	PIXEL_NUMBER_INQ	= 0x034;
static const uint32_t	TOTAL_BYTES_HI_INQ	= 0x038;
static const uint32_t	TOTAL_BYTES_LO_INQ	= 0x03c;
static const uint32_t	PACKET_PARA_INQ		= 0x040;
static const uint32_t	BYTE_PER_PACKET		= 0x044;
static const uint32_t	PACKET_PER_FRAME_INQ	= 0x048;
static const uint32_t	UNIT_POSITION_INQ	= 0x04c;
static const uint32_t	VALUE_SETTING		= 0x07c;

// NOTE: Two buffers are not enough under kernel-2.4.6 (2001.8.24).
static const u_int	NBUFFERS		= 4;

static const uint64_t	PointGrey_Feature_ID	= 0x00b09d000004ull;

/************************************************************************
*  class IIDCCamera							*
************************************************************************/
//! IIDCカメラノードを生成する
/*!
  \param type		カメラのタイプ
  \param uniqId		個々のカメラ固有の64bit ID. 同一のバスに
			複数のカメラが接続されている場合, これによって
			同定を行う. 0が与えられると, まだ IIDCCamera
			オブジェクトを割り当てられていないカメラのうち, 
			一番最初にみつかったものがこのオブジェクトと結び
			つけられる. オブジェクト生成後は, globalUniqueId()
			によってこの値を知ることができる.
  \param speed		FireWireバスの転送速度(1394bモードの場合は800Mbps)
  \param delay		FireWireカードの種類によっては, レジスタの読み書き
			(IIDCNode::readQuadlet(),
			IIDCNode::writeQuadlet())時に遅延を入れないと
			動作しないことがある. この遅延量をmicro second単位
			で指定する. (例: メルコのIFC-ILP3では1, DragonFly
			付属のボードでは0)
*/
IIDCCamera::IIDCCamera(Type type, uint64_t uniqId, Speed speed, u_int delay)
    :_node(getNode(type, uniqId, delay)),
     _cmdRegBase(_node->commandRegisterBase()),
     _acRegBase(inquireBasicFunction() & Advanced_Feature_Inq ?
		CSR_REGISTER_BASE + 4 * readQuadletFromRegister(0x480) : 0),
     _w(0), _h(0), _p(MONO_8), _img(0), _img_size(0),
     _bayer(YYYY), _littleEndian(false)
{
  // Set speed of isochronous transmission.
    setSpeed(speed);

  // Map video1394 buffer according to current format and frame rate.
    setFormatAndFrameRate(getFormat(), getFrameRate());

  // Get Bayer pattern supported by this camera.
    if (unlockAdvancedFeature(PointGrey_Feature_ID, 10))
    {
	switch (_node->readQuadlet(_acRegBase + 0x40))
	{
	  case RGGB:
	    _bayer = RGGB;
	    break;
	  case BGGR:
	    _bayer = BGGR;
	    break;
	  case GRBG:
	    _bayer = GRBG;
	    break;
	  case GBRG:
	    _bayer = GBRG;
	    break;
	}

	u_int	y16_data_format = _node->readQuadlet(_acRegBase + 0x48);
	if ((y16_data_format & 0x80000001) == 0x80000001)
	    _littleEndian = true;
    }
}

//! IIDCカメラオブジェクトを破壊する
/*!
  画像データ出力中であった場合は, それを停止する. 
*/
IIDCCamera::~IIDCCamera()
{
    stopContinuousShot();
    unembedTimestamp();
    delete _node;
}

//! IIDCカメラのglobal unique IDを返す
/*!
  \return	このIIDCカメラのglobal unique ID
*/
uint64_t
IIDCCamera::globalUniqueId() const
{
    return _node->globalUniqueId();
}
    
//! IIDCカメラの電源をonにする
/*!
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::powerOn()
{
    checkAvailability(Cam_Power_Cntl_Inq);
    writeQuadletToRegister(Camera_Power, 0x1u << 31);
    return *this;
}

//! IIDCカメラの電源をoffにする
/*!
  \return	このIIDCカメラオブジェクト. 
*/
IIDCCamera&
IIDCCamera::powerOff()
{
    checkAvailability(Cam_Power_Cntl_Inq);
    writeQuadletToRegister(Camera_Power, 0x00000000);
    return *this;
}

//! IIDCカメラのデータ転送速度を設定する
/*!
  \param speed	データ転送速度
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setSpeed(Speed speed)
{
    quadlet_t	quad;
    if (inquireBasicFunction() & I1394b_mode_Capability)
    {
	quad = ((0x1u << 15) | speed);
    }
    else
    {
	switch (speed)
	{
	  case SPD_100M:
	  case SPD_200M:
	  case SPD_400M:
	    quad = (speed << 24);
	    break;
	  default:
	    throw std::runtime_error("IIDCCaera::setSpeed: specified speed is not supported by non-IEEE1394b cameras!!");
	    break;
	}
    }

    const bool	cont = inContinuousShot();
    if (cont)
	stopContinuousShot();

    writeQuadletToRegister(ISO_Channel,
			   (readQuadletFromRegister(ISO_Channel)
			    & 0xf0003f00) | quad);

    if (cont)
	continuousShot();
    
    return *this;
}
    
//! IIDCカメラのデータ転送速度を返す
/*!
  \return	データ転送速度
*/
IIDCCamera::Speed
IIDCCamera::getSpeed() const
{
    quadlet_t	quad  = readQuadletFromRegister(ISO_Channel);
    quadlet_t	speed = (quad & (0x1u << 15) ? quad & 0x7
					     : (quad >> 24) & 0x3);
    
    switch (speed)
    {
      case SPD_100M:
	return SPD_100M;
      case SPD_200M:
	return SPD_200M;
      case SPD_400M:
	return SPD_400M;
      case SPD_800M:
	return SPD_800M;
      case SPD_1_6G:
	return SPD_1_6G;
      case SPD_3_2G:
	return SPD_3_2G;
    }

    throw std::runtime_error("Unknown speed!!");

    return SPD_100M;
}

//! 画像フォーマットとフレームレートを設定する
/*!
  画像データを出力中であった場合はそれを停止して設定を行うが, それが
  完了すれば出力を再開する.
  \param format	設定したい画像フォーマット
  \param rate	設定したいフレームレート
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setFormatAndFrameRate(Format format, FrameRate rate)
{
#ifdef DEBUG
    using namespace	std;
    
    cerr << "*** BEGIN [setFormatAndFrameRate] ***" << endl;
#endif
    checkAvailability(format, rate);
    const u_int	fmt  = (u_int(format) - u_int(YUV444_160x120)) / 0x20,
		mode = (u_int(format) - u_int(YUV444_160x120)) % 0x20 / 4;
    const bool	cont = inContinuousShot();
    
    if (cont)
	stopContinuousShot();
    
    u_int	rt = 0;
    for (u_int bit = FrameRate_1_875; bit != rate; bit >>= 1)
	++rt;
#ifdef DEBUG
    cerr << "  rt = " << dec << rt << endl;
#endif
    writeQuadletToRegister(Cur_V_Frm_Rate, rt   << 29);
    writeQuadletToRegister(Cur_V_Mode,	   mode << 29);
    writeQuadletToRegister(Cur_V_Format,   fmt  << 29);

  // Change buffer size and listen to the channel.
    u_int	packet_size = 0;
    switch (format)
    {
      case YUV444_160x120:
	_w = 160;
	_h = 120;
	_p = YUV_444;
	packet_size = 480 * sizeof(quadlet_t);
	break;
      case YUV422_320x240:
	_w = 320;
	_h = 240;
	_p = YUV_422;
	packet_size = 1280 * sizeof(quadlet_t);
	break;
      case YUV411_640x480:
	_w = 640;
	_h = 480;
	_p = YUV_411;
	packet_size = 3840 * sizeof(quadlet_t);
	break;
      case YUV422_640x480:
	_w = 640;
	_h = 480;
	_p = YUV_422;
	packet_size = 5120 * sizeof(quadlet_t);
	break;
      case RGB24_640x480:
	_w = 640;
	_h = 480;
	_p = RGB_24;
	packet_size = 7680 * sizeof(quadlet_t);
	break;
      case MONO8_640x480:
	_w = 640;
	_h = 480;
	_p = MONO_8;
	packet_size = 2560 * sizeof(quadlet_t);
	break;
      case MONO16_640x480:
	_w = 640;
	_h = 480;
	_p = MONO_16;
	packet_size = 5120 * sizeof(quadlet_t);
	break;
      case YUV422_800x600:
	_w = 800;
	_h = 600;
	_p = YUV_422;
	packet_size = 8000 * sizeof(quadlet_t);
	break;
      case RGB24_800x600:
	_w = 800;
	_h = 600;
	_p = RGB_24;
	packet_size = 12000 * sizeof(quadlet_t);
	break;
      case MONO8_800x600:
	_w = 800;
	_h = 600;
	_p = MONO_8;
	packet_size = 4000 * sizeof(quadlet_t);
	break;
      case YUV422_1024x768:
	_w = 1024;
	_h = 768;
	_p = YUV_422;
	packet_size = 12288 * sizeof(quadlet_t);
	break;
      case RGB24_1024x768:
	_w = 1024;
	_h = 768;
	_p = RGB_24;
	packet_size = 18432 * sizeof(quadlet_t);
	break;
      case MONO8_1024x768:
	_w = 1024;
	_h = 768;
	_p = MONO_8;
	packet_size = 6144 * sizeof(quadlet_t);
	break;
      case MONO16_800x600:
	_w = 800;
	_h = 600;
	_p = MONO_16;
	packet_size = 8000 * sizeof(quadlet_t);
	break;
      case MONO16_1024x768:
	_w = 1024;
	_h = 768;
	_p = MONO_16;
	packet_size = 12288 * sizeof(quadlet_t);
	break;
      case YUV422_1280x960:
	_w = 1280;
	_h = 960;
	_p = YUV_422;
	packet_size = 20480 * sizeof(quadlet_t);
	break;
      case RGB24_1280x960:
	_w = 1280;
	_h = 960;
	_p = RGB_24;
	packet_size = 30720 * sizeof(quadlet_t);
	break;
      case MONO8_1280x960:
	_w = 1280;
	_h = 960;
	_p = MONO_8;
	packet_size = 10240 * sizeof(quadlet_t);
	break;
      case YUV422_1600x1200:
	_w = 1600;
	_h = 1200;
	_p = YUV_422;
	packet_size = 32000 * sizeof(quadlet_t);
	break;
      case RGB24_1600x1200:
	_w = 1600;
	_h = 1200;
	_p = RGB_24;
	packet_size = 48000 * sizeof(quadlet_t);
	break;
      case MONO8_1600x1200:
	_w = 1600;
	_h = 1200;
	_p = MONO_8;
	packet_size = 16000 * sizeof(quadlet_t);
	break;
      case MONO16_1280x960:
	_w = 1280;
	_h = 960;
	_p = MONO_16;
	packet_size = 20480 * sizeof(quadlet_t);
	break;
      case MONO16_1600x1200:
	_w = 1600;
	_h = 1200;
	_p = MONO_16;
	packet_size = 32000 * sizeof(quadlet_t);
	break;
      case MONO8_640x480x2:
	_w = 640;
	_h = 480;
	_p = YUV_422;
	packet_size = 5120 * sizeof(quadlet_t);
	break;
      case Format_7_0:
      case Format_7_1:
      case Format_7_2:
      case Format_7_3:
      case Format_7_4:
      case Format_7_5:
      case Format_7_6:
      case Format_7_7:
      {
	const Format_7_Info	fmt7info = getFormat_7_Info(format);
	_w	    = fmt7info.width;
	_h	    = fmt7info.height;
	_p	    = fmt7info.pixelFormat;
	packet_size = setFormat_7_PacketSize(format);
	rt = 7;		// frameRateによってpacket_sizeを変えないようにする.
      }
	break;
      default:
	throw std::invalid_argument("IIDCCamera::setFormat: not implemented format!!");
	break;
    }
    packet_size >>= (7 - rt);	// frameRateによってpacket_sizeを変える.
    _img_size = _w * _h;
    switch (_p)
    {
      case YUV_444:
      case RGB_24:
	_img_size *= 3;
	break;
      case YUV_422:
      case MONO_16:
      case SIGNED_MONO_16:
      case RAW_16:
	_img_size *= 2;
	break;
      case RGB_48:
      case SIGNED_RGB_48:
	_img_size *= 6;
	break;
      case YUV_411:
	(_img_size *= 3) /= 2;
	break;
      default:
	break;
    }
  // buf_sizeをpacket_sizeの整数倍にしてからmapする.
    const u_int	 buf_size = packet_size * ((_img_size - 1) / packet_size + 1);
#ifdef DEBUG
    cerr << "  packetsize = " << packet_size << ", buf_size = " << buf_size
	 << endl;
#endif
    const u_char ch = _node->mapListenBuffer(packet_size, buf_size, NBUFFERS);

  // map時に割り当てられたチャンネル番号をカメラに設定する.
    quadlet_t	 quad = readQuadletFromRegister(ISO_Channel);
    (quad &= 0x0fffc0ff) |= ((ch << 28) | (ch << 8));
    writeQuadletToRegister(ISO_Channel, quad);

    if (cont)
	continuousShot();
    
#ifdef DEBUG
    cerr << "*** END [setFormatAndFrameRate] ***" << endl;
#endif
    return *this;
}

//! 現在カメラに設定されている画像フォーマットを返す
/*!
  \return	設定されている画像フォーマット
*/
IIDCCamera::Format
IIDCCamera::getFormat() const
{
    return
	uintToFormat(YUV444_160x120 +
		     ((readQuadletFromRegister(Cur_V_Mode)  >>29) & 0x7)*4 + 
		     ((readQuadletFromRegister(Cur_V_Format)>>29) & 0x7)*0x20);
}

//! 現在カメラに設定されているフレームレートを返す
/*!
  \return	設定されているフレームレート
*/
IIDCCamera::FrameRate
IIDCCamera::getFrameRate() const
{
    switch (getFormat())
    {
      case Format_7_0:
      case Format_7_1:
      case Format_7_2:
      case Format_7_3:
      case Format_7_4:
      case Format_7_5:
      case Format_7_6:
      case Format_7_7:
	return FrameRate_x;
      default:
	break;
    }
    return
	uintToFrameRate(0x1u << 
			(31 - ((readQuadletFromRegister(Cur_V_Frm_Rate) >> 29)
			       & 0x7)));
}

//! 指定されたFormat_7タイプのフォーマットの内容を返す
/*!
  ただし, 注目領域(ROI)の幅または高さが0の場合(カメラ電源投入直後など)は, 
  幅と高さをそれぞれの最小単位に設定し, その情報を返す. 
  \param format7 対象となるフォーマット(#Format_7_0 - #Format_7_7 のいずれか)
  \return	 指定されたフォーマットの内容
 */
IIDCCamera::Format_7_Info
IIDCCamera::getFormat_7_Info(Format format7)
{
    const nodeaddr_t	base = getFormat_7_BaseAddr(format7);
    quadlet_t		quad;
    Format_7_Info	fmt7info;
    quad = _node->readQuadlet(base + MAX_IMAGE_SIZE_INQ);
    fmt7info.maxWidth  = ((quad >> 16) & 0xffff);
    fmt7info.maxHeight = (quad & 0xffff);
    quad = _node->readQuadlet(base + UNIT_SIZE_INQ);
    fmt7info.unitWidth  = ((quad >> 16) & 0xffff);
    fmt7info.unitHeight = (quad & 0xffff);
    quad = _node->readQuadlet(base + UNIT_POSITION_INQ);
    fmt7info.unitU0 = ((quad >> 16) & 0xffff);
    if (fmt7info.unitU0 == 0)
	fmt7info.unitU0 = fmt7info.unitWidth;
    fmt7info.unitV0 = (quad & 0xffff);
    if (fmt7info.unitV0 == 0)
	fmt7info.unitV0 = fmt7info.unitHeight;
    quad = _node->readQuadlet(base + IMAGE_POSITION);
    fmt7info.u0 = ((quad >> 16) & 0xffff);
    fmt7info.v0 = (quad & 0xffff);
    quad = _node->readQuadlet(base + IMAGE_SIZE);
    fmt7info.width  = ((quad >> 16) & 0xffff);
    fmt7info.height = (quad & 0xffff);
    if (fmt7info.width == 0 || fmt7info.height == 0)
    {
	fmt7info.width  = fmt7info.unitWidth;
	fmt7info.height = fmt7info.unitHeight;
	_node->writeQuadlet(base + IMAGE_SIZE,
			    ((fmt7info.width << 16) & 0xffff0000) |
			    (fmt7info.height & 0xffff));
    }
    quad = _node->readQuadlet(base + COLOR_CODING_ID);
    const u_int colorCodingID = ((quad >> 24) & 0xff);
    if (colorCodingID > 31)
	throw std::runtime_error("IIDCCamera::getFormat_7_Info: Sorry, unsupported COLOR_CODING_ID!!");
    fmt7info.pixelFormat = uintToPixelFormat(0x1u << (31 - colorCodingID));
    quad = _node->readQuadlet(base + COLOR_CODING_INQ);
    fmt7info.availablePixelFormats = quad;
#ifdef DEBUG
    using namespace	std;
    
    cerr <<   "max size:    " << fmt7info.maxWidth  << 'x' << fmt7info.maxHeight
	 << "\nunit size:   " << fmt7info.unitWidth << 'x' << fmt7info.unitHeight
	 << "\norigin unit: " << fmt7info.unitU0 << 'x' << fmt7info.unitV0
	 << "\norigin:      " << fmt7info.u0	 << ',' << fmt7info.v0
	 << "\nsize:        " << fmt7info.width	 << 'x' << fmt7info.height
	 << endl;
#endif
    return fmt7info;
}

//! 指定されたFormat_7タイプのフォーマットについて, 注目領域(Region Of Interest)を設定する
/*!
  実際には, u0, v0, width, heightは, 予め決められた最小単位
  (Format_7_Info::unitU0, Format_7_Info::unitV0, Format_7_Info::unitWidth,
  Format_7_Info::unitHeight)の倍数となるように調整される. そこで, 注目領域は, 
  (u0, v0)と(u0 + width - 1, v0 + height - 1)を対角線とする矩形を含む
  最小領域となるように設定される. また, たとえwidthもしくはheightに0を指定
  しても, ROIの幅もしくは高さは, それぞれ Format_7_Info::unitWidth,
  Format_7_Info::unitHeight 以上に設定される.
  \param format7 対象となるフォーマット(#Format_7_0 - #Format_7_7 のいずれか)
  \param u0	 注目領域の左上隅の横座標
  \param v0	 注目領域の左上隅の縦座標
  \param width	 注目領域の幅
  \param height	 注目領域の高さ
  \return	 このIIDCカメラオブジェクト
 */
IIDCCamera&
IIDCCamera::setFormat_7_ROI(Format format7, u_int u0, u_int v0,
				u_int width, u_int height)
{
    const Format_7_Info	fmt7info = getFormat_7_Info(format7);

    const u_int	u1 = u0 + width;		    // 右端の希望値
    u0 = fmt7info.unitU0 * (u0 / fmt7info.unitU0);  // 左端を最小単位の倍数に
    while (u0 > fmt7info.maxWidth - fmt7info.unitWidth)
	u0 -= fmt7info.unitU0;	// 右に最小幅の余白ができるよう左端を修正
    width = fmt7info.unitWidth	// 右端希望値を含むよう幅を最小単位の倍数に
	  * (u1 - u0 > 0 ? (u1 - u0 - 1) / fmt7info.unitWidth + 1 : 1);
    while (u0 + width > fmt7info.maxWidth)
	width -= fmt7info.unitWidth;	// 最大幅に収まるよう幅を修正

    const u_int	v1 = v0 + height;		    // 下端の希望値
    v0 = fmt7info.unitV0 * (v0 / fmt7info.unitV0);  // 上端を最小単位の倍数に
    while (v0 > fmt7info.maxWidth - fmt7info.unitWidth)
	v0 -= fmt7info.unitV0;	 // 下に最小高さの余白ができるよう上端を修正
    height = fmt7info.unitHeight // 下端希望値を含むよう高さを最小単位の倍数に
	   * (v1 - v0 > 0 ? (v1 - v0 - 1) / fmt7info.unitHeight + 1 : 1);
    while (v0 + height > fmt7info.maxHeight)
	height -= fmt7info.unitHeight;	// 最大高さに収まるよう高さを修正

  // 画像出力中はROIを変更できないので, もしそうであれば停止する.
    const bool	cont = inContinuousShot();
    if (cont)
	stopContinuousShot();

  // ROIを指定する.
    const nodeaddr_t	base = getFormat_7_BaseAddr(format7);
    _node->writeQuadlet(base + IMAGE_POSITION,
			((u0 << 16) & 0xffff0000) | (v0 & 0xffff));
    _node->writeQuadlet(base + IMAGE_SIZE,
			((width << 16) & 0xffff0000) | (height & 0xffff));
    if (getFormat() == format7)
	setFormatAndFrameRate(format7, FrameRate_x);
    
    if (cont)
	continuousShot();
    
    return *this;
}

//! 指定されたFormat_7タイプのフォーマットについて, 画素形式を設定する
/*!
  \param format7	対象となるフォーマット(#Format_7_0 - #Format_7_7 の
			いずれか)
  \param pixelFormat	画素形式
  \return		このIIDCカメラオブジェクト
 */
IIDCCamera&
IIDCCamera::setFormat_7_PixelFormat(Format format7,
					PixelFormat pixelFormat)
{
    const Format_7_Info	fmt7info = getFormat_7_Info(format7);
    if (!(pixelFormat & fmt7info.availablePixelFormats))
	throw std::invalid_argument("IIDCCamera::setFormat_7_pixelFormat: unsupported pixel format!!");

  // 画像出力中はpixel formatを変更できないので, もしそうであれば停止する.
    const bool	cont = inContinuousShot();
    if (cont)
	stopContinuousShot();

  // pixel formatを指定する.
    const nodeaddr_t	base = getFormat_7_BaseAddr(format7);
    u_int		colorCodingID = 0;
    while ((0x1u << (31 - colorCodingID)) != pixelFormat)
	++colorCodingID;
    _node->writeQuadlet(base + COLOR_CODING_ID, (colorCodingID << 24));
    if (getFormat() == format7)
	setFormatAndFrameRate(format7, FrameRate_x);
    
    if (cont)
	continuousShot();
    
    return *this;
}

//! 指定された属性においてカメラがサポートする機能を返す
/*!
  \param feature	対象となる属性
  \return		サポートされている機能を #FeatureFunction 型の列挙値
			のorとして返す
 */
quadlet_t
IIDCCamera::inquireFeatureFunction(Feature feature) const
{
    u_int	n   = (u_int(feature) - 0x800) >> 2;
    quadlet_t	inq = 0;
    if (n < 32)		// FEATURE_HI
    {
	const u_int	Feature_Hi_Inq	= 0x404;
	inq = readQuadletFromRegister(Feature_Hi_Inq) & (0x1u << (31 - n));
    }
    else		// FEATURE_LO
    {
	const u_int	Feature_Lo_Inq	= 0x408;
	n -= 32;
	inq = readQuadletFromRegister(Feature_Lo_Inq) & (0x1u << (31 - n));
    }
    if (inq == 0)	// Check presence of feature.
	return 0;

    return readQuadletFromRegister(feature - 0x300);
}

//! 指定された属性を1回だけ自動設定する.
/*!
  本関数を呼ぶと, 指定した属性の自動設定が直ちに開始される. 自動設定が終了
  したかどうかは, #inOnePushOperation() で知ることができる.
  \param feature	自動設定したい属性
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::onePush(Feature feature)
{
    checkAvailability(feature, One_Push);
    writeQuadletToRegister(feature,
			   readQuadletFromRegister(feature) | One_Push);
    return *this;
}

//! 指定された属性をonにする
/*!
  \param feature	onにしたい属性
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::turnOn(Feature feature)
{
    checkAvailability(feature, OnOff);
    writeQuadletToRegister(feature, readQuadletFromRegister(feature) | ON_OFF);
    return *this;
}

//! 指定された属性をoffにする
/*!
  \param feature	offにしたい属性
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::turnOff(Feature feature)
{
    checkAvailability(feature, OnOff);
    writeQuadletToRegister(feature,
			   readQuadletFromRegister(feature) & ~ON_OFF);
    return *this;
}

//! 指定された属性を自動設定モードにする
/*!
  自動設定にすると, この属性の値は環境の変化に追従して継続的に自動的に調整
  される.
  \param feature	自動設定モードにしたい属性
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setAutoMode(Feature feature)
{
    checkAvailability(feature, Auto);
    writeQuadletToRegister(feature,
			   readQuadletFromRegister(feature) | A_M_Mode);
    return *this;
}

//! 指定された属性を手動設定モードにする
/*!
  \param feature	手動設定モードにしたい属性
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setManualMode(Feature feature)
{
    checkAvailability(feature, Manual);
    writeQuadletToRegister(feature,
			   readQuadletFromRegister(feature) & ~A_M_Mode);
    return *this;
}

//! 指定された属性の値を設定する
/*!
  #WHITE_BALANCE, #TRIGGER_MODE の値を設定することはできない. 代わりに
  setWhiteBalance(), setTriggerMode(), setTriggerPolarity() を用いること.
  \param feature	値を設定したい属性
  \param value		設定する値
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setValue(Feature feature, u_int value)
{
    if (feature == WHITE_BALANCE || feature == TRIGGER_MODE)
	throw std::invalid_argument("TU::IIDCCamera::setValue: cannot set WHITE_BALANCE/TRIGGER_MODE value using this method!!");
    checkAvailability(feature, Manual);
    if (feature == TEMPERATURE)
	writeQuadletToRegister(TEMPERATURE,
			       (readQuadletFromRegister(TEMPERATURE) &
				0xff000fff) | ((value & 0xfff) << 12));
    else
	writeQuadletToRegister(feature,
			       (readQuadletFromRegister(feature) &
				0xfffff000) | (value & 0xfff));
    return *this;
}

//! 指定された属性が1回だけの自動設定の最中であるか調べる
/*!
  \param feature	対象となる属性
  \return		#onePush() を行った属性値の自動設定が継続中であれば
			trueを, 終了していればfalseを返す.
*/
bool
IIDCCamera::inOnePushOperation(Feature feature) const
{
    checkAvailability(feature, One_Push);
    return readQuadletFromRegister(feature) & (0x1u << 26);
}

//! 指定された属性がonになっているか調べる
/*!
  \param feature	対象となる属性
  \return		onになっていればtrueを, そうでなければfalseを返す.
*/
bool
IIDCCamera::isTurnedOn(Feature feature) const
{
    checkAvailability(feature, OnOff);
    return readQuadletFromRegister(feature) & (0x1u << 25);
}

//! 指定された属性が自動設定モードになっているか調べる
/*!
  \param feature	対象となる属性
  \return		自動設定モードになっていればtrueを, そうでなければ
			falseを返す.
*/
bool
IIDCCamera::isAuto(Feature feature) const
{
    checkAvailability(feature, Auto);
    return readQuadletFromRegister(feature) & (0x1u << 24);
}

//! 指定された属性がとり得る値の範囲を調べる
/*!
  \param feature	対象となる属性
  \param min		とり得る値の最小値が返される. 
  \param max		とり得る値の最大値が返される. 
*/
void
IIDCCamera::getMinMax(Feature feature, u_int& min, u_int& max) const
{
    
    quadlet_t	quad = checkAvailability(feature, Presence);
    min = (quad >> 12) & 0xfff;
    max = quad & 0xfff;
}

//! 指定された属性の現在の値を調べる
/*!
  feature = #TEMPERATURE の場合は, #setValue() で設定した目標値ではなく, 
  実際値が返される. 目標値を得るには, #getAimedTemperature() を用いる.
  また, #WHITE_BALANCE, #TRIGGER_MODE の値を知ることはできない. 代わり
  に #getWhiteBalance(), #getTriggerMode(), #getTriggerPolarity() を用いる
  こと.
  \param feature	対象となる属性
  \return		現在の値
*/
u_int
IIDCCamera::getValue(Feature feature) const
{
    if (feature == WHITE_BALANCE || feature == TRIGGER_MODE)
	throw std::invalid_argument("TU::IIDCCamera::getValue: cannot get WHITE_BALANCE/TRIGGER_MODE value using this method!!");
    checkAvailability(feature, ReadOut);
    return readQuadletFromRegister(feature) & 0xfff;	// 12bit value.
}

//! ホワイトバランスの値を設定する
/*!
  \param ub		設定するU/B値
  \param vr		設定するV/R値
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setWhiteBalance(u_int ub, u_int vr)
{
    checkAvailability(WHITE_BALANCE, Manual);
    writeQuadletToRegister(WHITE_BALANCE,
			   (readQuadletFromRegister(WHITE_BALANCE) &
			    0xff000000) | ((ub & 0xfff) << 12) | (vr & 0xfff));
    return *this;
}

//! ホワイトバランスの値を調べる
/*!
  \param ub		U/B値が返される
  \param vr		V/R値が返される
*/
void
IIDCCamera::getWhiteBalance(u_int &ub, u_int& vr) const
{
    checkAvailability(WHITE_BALANCE, ReadOut);
    quadlet_t	quad = readQuadletFromRegister(WHITE_BALANCE);
    ub = (quad >> 12) & 0xfff;
    vr = quad & 0xfff;
}

//! 色温度の目標値を調べる
/*!
  色温度の実際値を知るには, 代わりに #getValue() を用いよ.
  \return	設定されている目標値が返される.
*/
u_int
IIDCCamera::getAimedTemperature() const
{
    checkAvailability(TEMPERATURE, ReadOut);
    return (readQuadletFromRegister(TEMPERATURE) >> 12) & 0xfff;
}

//! トリガモードを設定する
/*!
  実際にカメラが外部トリガによって駆動されるためには, この関数でモード設定
  を行った後に #turnOn(#TRIGGER_MODE) を行わなければならない.
  \param mode	設定したいトリガモード
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setTriggerMode(TriggerMode mode)
{
    checkAvailability(TRIGGER_MODE, mode);
    writeQuadletToRegister(TRIGGER_MODE,
			   (readQuadletFromRegister(TRIGGER_MODE) & ~0xf0000) |
			   triggerModeValue(mode) << 16);
    return *this;
}

//! 現在設定されているトリガモードを調べる
/*!
  \return	現在設定されているトリガモード
*/
IIDCCamera::TriggerMode
IIDCCamera::getTriggerMode() const
{
    checkAvailability(TRIGGER_MODE, ReadOut);
    return uintToTriggerMode(
	       triggerModeInq(
		   (readQuadletFromRegister(TRIGGER_MODE) >> 16) & 0xf));
}

//! トリガ信号の極性を設定する
/*!
  \param polarity	設定したい極性
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::setTriggerPolarity(TriggerPolarity polarity)
{
    const quadlet_t	Polarity_Inq = 0x1u << 25;
    checkAvailability(TRIGGER_MODE, Polarity_Inq);
    writeQuadletToRegister(TRIGGER_MODE, (readQuadletFromRegister(TRIGGER_MODE)
					  & ~HighActiveInput) | polarity);
    return *this;
}

//! 現在設定されているトリガ信号の極性を調べる
/*!
  \return	現在設定されているトリガ信号の極性
*/
IIDCCamera::TriggerPolarity
IIDCCamera::getTriggerPolarity() const
{
    checkAvailability(TRIGGER_MODE, ReadOut);
    if (readQuadletFromRegister(TRIGGER_MODE) & HighActiveInput)
	return HighActiveInput;
    else
	return LowActiveInput;
}

//! カメラからの画像の連続的出力を開始する
/*!
  #TRIGGER_MODE がonであれば, 撮影のタイミングは外部トリガ信号によって制御さ
  れる. 
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::continuousShot()
{
    if (!inContinuousShot())
	writeQuadletToRegister(ISO_EN, 0x1u << 31);
    return *this;
}

//! カメラからの画像の連続的出力を停止する
/*!
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::stopContinuousShot()
{
    if (inContinuousShot())
    {
	writeQuadletToRegister(ISO_EN, 0x0);
      //flushListenBuffer();
	_img = 0;
      // 再び continuoutShot() した時に	captureRaw()で使用するので, 
      // _img_size の値は0にせずに保持する.
    }
    return *this;
}

//! カメラから画像が出力中であるか調べる
/*!
  \return	画像出力中であればtrueを, そうでなければfalseを返す. 
*/
bool
IIDCCamera::inContinuousShot() const
{
    return readQuadletFromRegister(ISO_EN) & (0x1u << 31);
}

//! 画像を1枚だけ撮影してそれを出力する
/*!
  画像を連続的に出力中であれば, それを停止した後にあらためて1枚だけ撮影する. 
  #TRIGGER_MODE がonであれば, 撮影のタイミングは外部トリガ信号によって制御さ
  れる.
  \return	このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::oneShot()
{
    checkAvailability(One_Shot_Inq);
    stopContinuousShot();
    writeQuadletToRegister(One_Shot, 0x1u << 31);
    return *this;
}

//! 画像を指定された枚数だけ撮影してそれを出力する
/*!
  画像を連続的に出力中であれば, それを停止した後にあらためて撮影を開始する. 
  #TRIGGER_MODE がonであれば, 撮影のタイミングは外部トリガ信号によって制御さ
  れる.
  \param nframes	撮影したい枚数
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::multiShot(u_short nframes)
{
    checkAvailability(Multi_Shot_Inq);
    stopContinuousShot();
    writeQuadletToRegister(One_Shot, (0x1u << 30) | (nframes & 0xffff));
    return *this;
}

//! 現在のカメラの設定を指定されたメモリチャンネルに記憶する
/*!
  IIDCカメラの一部には, カメラに設定した画像フォーマットや属性値などを
  カメラ内部のメモリチャンネルに記憶できるものがある. 
  \param mem_ch		値を記憶するメモリチャンネル番号. 0以上の値をとり, 
			最大値は #getMemoryChannelMax() で調べられる. 
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::saveConfig(u_int mem_ch)
{
    u_int	max = getMemoryChannelMax();
    if (mem_ch == 0 || mem_ch > max)
	throw std::invalid_argument("TU::IIDCCamera::saveConfig: invalid memory channel!!");
    writeQuadletToRegister(Mem_Save_Ch, mem_ch << 28);
    writeQuadletToRegister(Memory_Save, 0x1u << 31);
    while ((readQuadletFromRegister(Memory_Save) & (0x1u << 31)) != 0)
	;
    return *this;
}

//! 指定されたメモリチャンネルに記憶された値をカメラに設定する
/*!
  IIDCカメラの一部には, カメラに設定した画像フォーマットや属性値などを
  カメラ内部のメモリチャンネルに記憶できるものがある.
  \param mem_ch		設定したい値を記憶しているメモリチャンネル番号. 0以上
			の値をとり, 最大値は #getMemoryChannelMax() で調べら
			れる.
  \return		このIIDCカメラオブジェクト
*/
IIDCCamera&
IIDCCamera::restoreConfig(u_int mem_ch)
{
    u_int	max = getMemoryChannelMax();
    if (mem_ch > max)
	throw std::invalid_argument("TU::IIDCCamera::restoreConfig: invalid memory channel!!");
    writeQuadletToRegister(Cur_Mem_Ch, mem_ch << 28);
    return *this;
}

//! メモリチャンネルの最大値を調べる
/*!
  IIDCカメラの一部には, カメラに設定した画像フォーマットや属性値などを
  カメラ内部のメモリチャンネルに記憶できるものがある. 
  \return 	メモリチャンネル番号の最大値
*/
u_int
IIDCCamera::getMemoryChannelMax() const
{
    return (inquireBasicFunction() & 0xf);
}

/*
 *  Capture stuffs.
 */
#ifdef HAVE_LIBTUTOOLS__
//! IIDCカメラから出力された画像1枚分のデータを適当な形式に変換して取り込む
/*!
  テンプレートパラメータTは, 格納先の画像の画素形式を表す. なお, 本関数を
  呼び出す前に #snap() によってカメラからの画像を保持しておかなければならない.
  \param image	画像データを格納する画像オブジェクト. 画像の幅と高さは, 
		現在カメラに設定されている画像サイズに合わせて自動的に
		設定される. また, カメラに設定されたフォーマットの画素形式
		が画像のそれに一致しない場合は, 自動的に変換が行われる.
		サポートされている画素形式Tは, u_char, short, float, double,
		RGB, RGBA, BGR,	ABGR, YUV444, YUV422, YUV411 のいずれかである.
		また, サポートされている変換は以下のとおりであり, カメラの
		画素形式がこれ以外に設定されている場合はstd::domain_error
		例外が送出される.
		    -# #YUV_444 -> T
		    -# #YUV_422 -> T
		    -# #YUV_411 -> T
		    -# #RGB_24 -> T (YUV444, YUV422, YUV411 を除く) 
		    -# #MONO_8 -> T
		    -# #MONO_16 -> T
  \return	このIIDCカメラオブジェクト
*/
template <class T> const IIDCCamera&
IIDCCamera::operator >>(Image<T>& image) const
{
    if (_img == 0)
	throw std::runtime_error("TU::IIDCCamera::operator >>: no images snapped!!");
  // Transfer image data from current buffer.
    image.resize(height(), width());
    switch (pixelFormat())
    {
      case YUV_444:
      {
	const YUV444*	src = (const YUV444*)_img;
	for (u_int v = 0; v < image.height(); ++v)
	    src = image[v].copy(src);
      }
	break;
      case YUV_422:
      {
	const YUV422*	src = (const YUV422*)_img;
	for (u_int v = 0; v < image.height(); ++v)
	    src = image[v].copy(src);
      }
	break;
      case YUV_411:
      {
	const YUV411*	src = (const YUV411*)_img;
	for (u_int v = 0; v < image.height(); ++v)
	    src = image[v].copy(src);
      }
	break;
      case RGB_24:
      {
	const RGB*	src = (const RGB*)_img;
	for (u_int v = 0; v < image.height(); ++v)
	    src = image[v].copy(src);
      }
	break;
      case MONO_8:
      case RAW_8:
      {
	const u_char*	src = _img;
	for (u_int v = 0; v < image.height(); ++v)
	    src = image[v].copy(src);
      }
	break;
      case MONO_16:
      case RAW_16:
	if (_littleEndian)
	{
	    const u_short*	src = (const u_short*)_img;
	    for (u_int v = 0; v < image.height(); ++v)
		src = image[v].copy(src);
	}
	else
	{
	    const Mono16*	src = (const Mono16*)_img;
	    for (u_int v = 0; v < image.height(); ++v)
		src = image[v].copy(src);
	}
	break;
      case SIGNED_MONO_16:
	if (_littleEndian)
	{
	    const short*	src = (const short*)_img;
	    for (u_int v = 0; v < image.height(); ++v)
		src = image[v].copy(src);
	}
	else
	{
	    const Mono16*	src = (const Mono16*)_img;
	    for (u_int v = 0; v < image.height(); ++v)
		src = image[v].copy(src);
	}
	break;
      default:
	throw std::domain_error("TU::IIDCCamera::operator >>: unknown pixel format!!");
	break;
    }

    return *this;
}

//! IIDCカメラから出力された画像をRGB形式カラー画像として取り込む
/*!
  #operator >>() との違いは, カメラが #Bayer パターンをサポートしたカメラで
  ある場合, #Bayer パターンからRGB形式への変換を行うことである.
  テンプレートパラメータTは, 格納先の画像の画素形式を表す. なお, 本関数を
  呼び出す前に #snap() によってカメラからの画像を保持しておかなければならない.
  \param image	画像データを格納する画像オブジェクト. 画像の幅と高さは,
		現在カメラに設定されている画像サイズに合わせて自動的に
		設定される. サポートされている画素形式Tは RGB, RGBA,
		BGR, ABGR のいずれかである.
  \return	このIIDCカメラオブジェクト
*/
template <class T> const IIDCCamera&
IIDCCamera::captureRGBImage(Image<T>& image) const
{
    if (_img == 0)
	throw std::runtime_error("TU::IIDCCamera::captureRGBImage: no images snapped!!");
  // Transfer image data from current buffer.
    image.resize(height(), width());
    switch (pixelFormat())
    {
      case MONO_8:
	switch (_bayer)
	{
	  case RGGB:
	  {
	    const u_char*	p = bayerRGGB2x2(_img, &image[0][0], width());
	    int			v = 1;
	    while (v < image.height() - 1)	// 中間の行を処理
	    {
		p = bayerRGGBOdd3x3 (p, &image[v++][0], width());
		p = bayerRGGBEven3x3(p, &image[v++][0], width());
	    }
	    bayerRGGB2x2(p - width(), &image[v][0], width());
	  }
	    break;

	  case BGGR:
	  {
	    const u_char*	p = bayerBGGR2x2(_img, &image[0][0], width());
	    int			v = 1;
	    while (v < image.height() - 1)	// 中間の行を処理
	    {
		p = bayerBGGROdd3x3 (p, &image[v++][0], width());
		p = bayerBGGREven3x3(p, &image[v++][0], width());
	    }
	    bayerBGGR2x2(p - width(), &image[v][0], width());
	  }
	    break;

	  case GRBG:
	  {
	    const u_char*	p = bayerGRBG2x2(_img, &image[0][0], width());
	    int			v = 1;
	    while (v < image.height() - 1)	// 中間の行を処理
	    {
		p = bayerGRBGOdd3x3 (p, &image[v++][0], width());
		p = bayerGRBGEven3x3(p, &image[v++][0], width());
	    }
	    bayerGRBG2x2(p - width(), &image[v][0], width());
	  }
	    break;

	  case GBRG:
	  {
	    const u_char*	p = bayerGBRG2x2(_img, &image[0][0], width());
	    int			v = 1;
	    while (v < image.height() - 1)	// 中間の行を処理
	    {
		p = bayerGBRGOdd3x3 (p, &image[v++][0], width());
		p = bayerGBRGEven3x3(p, &image[v++][0], width());
	    }
	    bayerGBRG2x2(p - width(), &image[v][0], width());
	  }
	    break;

	  default:
	    *this >> image;
	    break;
	}
	break;
	
      case MONO_16:
	switch (_bayer)
	{
	  case RGGB:
	    if (_littleEndian)
	    {
		const u_short*	p = bayerRGGB2x2((const u_short*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerRGGBOdd3x3 (p, &image[v++][0], width());
		    p = bayerRGGBEven3x3(p, &image[v++][0], width());
		}
		bayerRGGB2x2(p - width(), &image[v][0], width());
	    }
	    else
	    {
		const Mono16*	p = bayerRGGB2x2((const Mono16*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerRGGBOdd3x3 (p, &image[v++][0], width());
		    p = bayerRGGBEven3x3(p, &image[v++][0], width());
		}
		bayerRGGB2x2(p - width(), &image[v][0], width());
	    }
	    break;

	  case BGGR:
	    if (_littleEndian)
	    {
		const u_short*	p = bayerBGGR2x2((const u_short*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerBGGROdd3x3 (p, &image[v++][0], width());
		    p = bayerBGGREven3x3(p, &image[v++][0], width());
		}
		bayerBGGR2x2(p - width(), &image[v][0], width());
	    }
	    else
	    {
		const Mono16*	p = bayerBGGR2x2((const Mono16*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerBGGROdd3x3 (p, &image[v++][0], width());
		    p = bayerBGGREven3x3(p, &image[v++][0], width());
		}
		bayerBGGR2x2(p - width(), &image[v][0], width());
	    }
	    break;

	  case GRBG:
	    if (_littleEndian)
	    {
		const u_short*	p = bayerGRBG2x2((const u_short*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerGRBGOdd3x3 (p, &image[v++][0], width());
		    p = bayerGRBGEven3x3(p, &image[v++][0], width());
		}
		bayerGRBG2x2(p - width(), &image[v][0], width());
	    }
	    else
	    {
		const Mono16*	p = bayerGRBG2x2((const Mono16*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerGRBGOdd3x3 (p, &image[v++][0], width());
		    p = bayerGRBGEven3x3(p, &image[v++][0], width());
		}
		bayerGRBG2x2(p - width(), &image[v][0], width());
	    }
	    break;

	  case GBRG:
	    if (_littleEndian)
	    {
		const u_short*	p = bayerGBRG2x2((const u_short*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerGBRGOdd3x3 (p, &image[v++][0], width());
		    p = bayerGBRGEven3x3(p, &image[v++][0], width());
		}
		bayerGBRG2x2(p - width(), &image[v][0], width());
	    }
	    else
	    {
		const Mono16*	p = bayerGBRG2x2((const Mono16*)_img,
						 &image[0][0], width());
		int		v = 1;
		while (v < image.height() - 1)	// 中間の行を処理
		{
		    p = bayerGBRGOdd3x3 (p, &image[v++][0], width());
		    p = bayerGBRGEven3x3(p, &image[v++][0], width());
		}
		bayerGBRG2x2(p - width(), &image[v][0], width());
	    }
	    break;

	  default:
	    *this >> image;
	    break;
	}
	break;

      default:
	*this >> image;
	break;
    }

    return *this;
}
#else
struct RGB
{
    u_char	r, g, b;
};
#endif	// HAVE_LIBTUTOOLS__

//! IIDCカメラから出力された画像1枚分のデータをなんら変換を行わずに取り込む
/*!
  本関数を呼び出す前に #snap() によってカメラからの画像を保持しておかなければ
  ならない.
  \param image	画像データの格納領域へのポインタ. #width(), #height() および
		#pixelFormat() を用いて画像のサイズと画素の形式を調べて
		画像1枚分の領域を確保しておくのは, ユーザの責任である.
  \return	このIIDCカメラオブジェクト
*/
const IIDCCamera&
IIDCCamera::captureRaw(void* image) const
{
    if (_img == 0)
	throw std::runtime_error("TU::IIDCCamera::captureRaw: no images snapped!!");
  // Transfer image data from current buffer.
    memcpy(image, _img, _img_size);

    return *this;
}

//! IIDCカメラから出力されたBayerパターン画像1枚分のデータをRGB形式に変換して取り込む
/*!
  本関数を呼び出す前に #snap() によってカメラからの画像を保持しておかなければ
  ならない.
  \param image	画像データの格納領域へのポインタ. #width(), #height() および
		#pixelFormat() を用いて画像のサイズと画素の形式を調べて
		画像1枚分の領域を確保しておくのは, ユーザの責任である.
		画像データは, 各画素毎に R, G, B (各 1 byte)の順で格納され
		る. カメラの画素形式が #MONO_8 または #MONO_16 以外に設定され
		ている場合はstd::domain_error例外が送出される.
  \return	このIIDCカメラオブジェクト
*/
const IIDCCamera&
IIDCCamera::captureBayerRaw(void* image) const
{
    if (_img == 0)
	throw std::runtime_error("TU::IIDCCamera::captureBayerRaw: no images snapped!!");

  // Transfer image data from current buffer.
    switch (pixelFormat())
    {
      case MONO_8:
	switch (_bayer)
	{
	  case RGGB:
	  {
	    RGB*		rgb = (RGB*)image;
	    const u_char*	p = bayerRGGB2x2(_img, rgb, width());
	    rgb += width();
	    for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理 
	    {
		p = bayerRGGBOdd3x3 (p, rgb, width());
		rgb += width();
		p = bayerRGGBEven3x3(p, rgb, width());
		rgb += width();
	    }
	    bayerRGGB2x2(p - width(), rgb, width());
	  }
	    break;

	  case BGGR:
	  {
	    RGB*		rgb = (RGB*)image;
	    const u_char*	p = bayerBGGR2x2(_img, rgb, width());
	    rgb += width();
	    for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
	    {
		p = bayerBGGROdd3x3 (p, rgb, width());
		rgb += width();
		p = bayerBGGREven3x3(p, rgb, width());
		rgb += width();
	    }
	    bayerBGGR2x2(p - width(), rgb, width());
	  }
	    break;

	  case GRBG:
	  {
	    RGB*		rgb = (RGB*)image;
	    const u_char*	p = bayerGRBG2x2(_img, rgb, width());
	    rgb += width();
	    for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
	    {
		p = bayerGRBGOdd3x3 (p, rgb, width());
		rgb += width();
		p = bayerGRBGEven3x3(p, rgb, width());
		rgb += width();
	    }
	    bayerGRBG2x2(p - width(), rgb, width());
	  }
	    break;

	  case GBRG:
	  {
	    RGB*		rgb = (RGB*)image;
	    const u_char*	p = bayerGBRG2x2(_img, rgb, width());
	    rgb += width();
	    for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
	    {
		p = bayerGBRGOdd3x3 (p, rgb, width());
		rgb += width();
		p = bayerGBRGEven3x3(p, rgb, width());
		rgb += width();
	    }
	    bayerGBRG2x2(p - width(), rgb, width());
	  }
	    break;

	  default:
	  {
	    RGB*		rgb = (RGB*)image;
	    const u_char*	p = _img;
	    for (int n = width() * height(); n-- > 0; )
	    {
		rgb->r = rgb->g = rgb->b = *p++;
		++rgb;
	    }
	  }
	    break;
	}
	break;
	
      case MONO_16:
	switch (_bayer)
	{
	  case RGGB:
	    if (_littleEndian)
	    {
		RGB*		rgb = (RGB*)image;
		const u_short*	p = bayerRGGB2x2((const u_short*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerRGGBOdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerRGGBEven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerRGGB2x2(p - width(), rgb, width());
	    }
	    else
	    {
		RGB*		rgb = (RGB*)image;
		const Mono16*	p = bayerRGGB2x2((const Mono16*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerRGGBOdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerRGGBEven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerRGGB2x2(p - width(), rgb, width());
	    }
	    break;

	  case BGGR:
	    if (_littleEndian)
	    {
		RGB*		rgb = (RGB*)image;
		const u_short*	p = bayerBGGR2x2((const u_short*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerBGGROdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerBGGREven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerBGGR2x2(p - width(), rgb, width());
	    }
	    else
	    {
		RGB*		rgb = (RGB*)image;
		const Mono16*	p = bayerBGGR2x2((const Mono16*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerBGGROdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerBGGREven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerBGGR2x2(p - width(), rgb, width());
	    }
	    break;
	    
	  case GRBG:
	    if (_littleEndian)
	    {
		RGB*		rgb = (RGB*)image;
		const u_short*	p = bayerGRBG2x2((const u_short*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerGRBGOdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerGRBGEven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerGRBG2x2(p - width(), rgb, width());
	    }
	    else
	    {
		RGB*		rgb = (RGB*)image;
		const Mono16*	p = bayerGRBG2x2((const Mono16*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerGRBGOdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerGRBGEven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerGRBG2x2(p - width(), rgb, width());
	    }
	    break;
	    
	  case GBRG:
	    if (_littleEndian)
	    {
		RGB*		rgb = (RGB*)image;
		const u_short*	p = bayerGBRG2x2((const u_short*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerGBRGOdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerGBRGEven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerGBRG2x2(p - width(), rgb, width());
	    }
	    else
	    {
		RGB*		rgb = (RGB*)image;
		const Mono16*	p = bayerGBRG2x2((const Mono16*)_img, rgb, width());
		rgb += width();
		for (int n = height(); (n -= 2) > 0; )	// 中間の行を処理
		{
		    p = bayerGBRGOdd3x3 (p, rgb, width());
		    rgb += width();
		    p = bayerGBRGEven3x3(p, rgb, width());
		    rgb += width();
		}
		bayerGBRG2x2(p - width(), rgb, width());
	    }
	    break;
	    
	  default:
	    if (_littleEndian)
	    {
		RGB*		rgb = (RGB*)image;
		const u_short*	p = (const u_short*)_img;
		for (int n = width() * height(); n-- > 0; )
		{
		    rgb->r = rgb->g = rgb->b = *p++;
		    ++rgb;
		}
	    }
	    else
	    {
		RGB*		rgb = (RGB*)image;
		const Mono16*	p = (const Mono16*)_img;
		for (int n = width() * height(); n-- > 0; )
		{
		    rgb->r = rgb->g = rgb->b = *p++;
		    ++rgb;
		}
	    }
	    break;
	}
	break;
	
      default:
	throw std::domain_error("TU::IIDCCamera::captureBayerRaw: must be MONO_8 or MONO_16 format!!");
	break;
    }

    return *this;
}

//! IIDCカメラからの画像の先頭4byteにタイムスタンプを埋め込む
/*!
  Point Grey社のカメラのみに有効．
  \return	このIIDCカメラオブジェクト. 
*/
IIDCCamera&
IIDCCamera::embedTimestamp()
{
    if (unlockAdvancedFeature(PointGrey_Feature_ID, 10))
    {
	quadlet_t	val = _node->readQuadlet(_acRegBase + 0x02f8);
	if (val & (0x1u << 31))
	    _node->writeQuadlet(_acRegBase + 0x02f8, val | 0x1u);
    }
    return *this;
}

//! IIDCカメラからの画像の先頭4byteへのタイムスタンプ埋め込みを解除する
/*!
  Point Grey社のカメラのみに有効．
  \return	このIIDCカメラオブジェクト. 
*/
IIDCCamera&
IIDCCamera::unembedTimestamp()
{
    if (unlockAdvancedFeature(PointGrey_Feature_ID, 10))
    {
	quadlet_t	val = _node->readQuadlet(_acRegBase + 0x02f8);
	if (val & (0x1u << 31))
	    _node->writeQuadlet(_acRegBase + 0x02f8, val & ~0x1u);
    }
    return *this;
}

//! unsinged intの値を同じビットパターンを持つ #Format に直す
/*!
  \param format	#Format に直したいunsigned int値
  \return	#Format 型のenum値
 */
IIDCCamera::Format
IIDCCamera::uintToFormat(u_int format)
{
    switch (format)
    {
      case YUV444_160x120:
	return YUV444_160x120;
      case YUV422_320x240:
	return YUV422_320x240;
      case YUV411_640x480:
	return YUV411_640x480;
      case YUV422_640x480:
	return YUV422_640x480;
      case RGB24_640x480:
	return RGB24_640x480;
      case MONO8_640x480:
	return MONO8_640x480;
      case MONO16_640x480:
	return MONO16_640x480;
      case YUV422_800x600:
	return YUV422_800x600;
      case RGB24_800x600:
	return RGB24_800x600;
      case MONO8_800x600:
	return MONO8_800x600;
      case YUV422_1024x768:
	return YUV422_1024x768;
      case RGB24_1024x768:
	return RGB24_1024x768;
      case MONO8_1024x768:
	return MONO8_1024x768;
      case MONO16_800x600:
	return MONO16_800x600;
      case MONO16_1024x768:
	return MONO16_1024x768;
      case YUV422_1280x960:
	return YUV422_1280x960;
      case RGB24_1280x960:
	return RGB24_1280x960;
      case MONO8_1280x960:
	return MONO8_1280x960;
      case YUV422_1600x1200:
	return YUV422_1600x1200;
      case RGB24_1600x1200:
	return RGB24_1600x1200;
      case MONO8_1600x1200:
	return MONO8_1600x1200;
      case MONO16_1280x960:
	return MONO16_1280x960;
      case MONO16_1600x1200:
	return MONO16_1600x1200;
      case Format_5_0:
	return Format_5_0;
      case MONO8_640x480x2:
	return MONO8_640x480x2;
      case Format_5_2:
	return Format_5_2;
      case Format_5_3:
	return Format_5_3;
      case Format_5_4:
	return Format_5_4;
      case Format_5_5:
	return Format_5_5;
      case Format_5_6:
	return Format_5_6;
      case Format_5_7:
	return Format_5_7;
      case Format_7_0:
	return Format_7_0;
      case Format_7_1:
	return Format_7_1;
      case Format_7_2:
	return Format_7_2;
      case Format_7_3:
	return Format_7_3;
      case Format_7_4:
	return Format_7_4;
      case Format_7_5:
	return Format_7_5;
      case Format_7_6:
	return Format_7_6;
      case Format_7_7:
	return Format_7_7;
    }

    throw std::invalid_argument("Unknown format!!");
    
    return YUV444_160x120;
}

//! unsinged intの値を同じビットパターンを持つ #FrameRate に直す
/*!
  \param rate	#FrameRate に直したいunsigned int値
  \return	#FrameRate 型のenum値
 */
IIDCCamera::FrameRate
IIDCCamera::uintToFrameRate(u_int rate)
{
    switch (rate)
    {
      case FrameRate_1_875:
	return FrameRate_1_875;
      case FrameRate_3_75:
	return FrameRate_3_75;
      case FrameRate_7_5:
	return FrameRate_7_5;
      case FrameRate_15:
	return FrameRate_15;
      case FrameRate_30:
	return FrameRate_30;
      case FrameRate_60:
	return FrameRate_60;
      case FrameRate_120:
	return FrameRate_120;
      case FrameRate_240:
	return FrameRate_240;
      case FrameRate_x:
	return FrameRate_x;
    }

    throw std::invalid_argument("Unknown frame rate!!");
    
    return FrameRate_1_875;
}

//! unsinged intの値を同じビットパターンを持つ #Feature に直す
/*!
  \param feature	#Feature に直したいunsigned int値
  \return		#Feature 型のenum値
 */
IIDCCamera::Feature
IIDCCamera::uintToFeature(u_int feature)
{
    switch (feature)
    {
      case BRIGHTNESS:
	return BRIGHTNESS;
      case AUTO_EXPOSURE:
	return AUTO_EXPOSURE;
      case SHARPNESS:
	return SHARPNESS;
      case WHITE_BALANCE:
	return WHITE_BALANCE;
      case HUE:
	return HUE;
      case SATURATION:
	return SATURATION;
      case GAMMA:
	return GAMMA;
      case SHUTTER:
	return SHUTTER;
      case GAIN:
	return GAIN;
      case IRIS:
	return IRIS;
      case FOCUS:
	return FOCUS;
      case TEMPERATURE:
	return TEMPERATURE;
      case TRIGGER_MODE:
	return TRIGGER_MODE;
      case ZOOM:
	return ZOOM;
      case PAN:
	return PAN;
      case TILT:
	return TILT;
      case OPTICAL_FILTER:
	return OPTICAL_FILTER;
      case CAPTURE_SIZE:
	return CAPTURE_SIZE;
      case CAPTURE_QUALITY:
	return CAPTURE_QUALITY;
    }

    throw std::invalid_argument("Unknown feature!!");
    
    return BRIGHTNESS;
}

//! unsinged intの値を同じビットパターンを持つ #TriggerMode に直す
/*!
  \param triggerMode	#TriggerMode に直したいunsigned int値
  \return		#TriggerMode 型のenum値
 */
IIDCCamera::TriggerMode
IIDCCamera::uintToTriggerMode(u_int triggerMode)
{
    switch (triggerMode)
    {
      case Trigger_Mode0:
	return Trigger_Mode0;
      case Trigger_Mode1:
	return Trigger_Mode1;
      case Trigger_Mode2:
	return Trigger_Mode2;
      case Trigger_Mode3:
	return Trigger_Mode3;
      case Trigger_Mode4:
	return Trigger_Mode4;
      case Trigger_Mode5:
	return Trigger_Mode5;
      case Trigger_Mode14:
	return Trigger_Mode14;
    }

    throw std::invalid_argument("Unknown trigger mode!!");
    
    return Trigger_Mode0;
}
 
//! unsinged intの値を同じビットパターンを持つ #PixelFormat に直す
/*!
  \param pixelFormat	#PixelFormat に直したいunsigned int値
  \return		#PixelFormat 型のenum値
 */
IIDCCamera::PixelFormat
IIDCCamera::uintToPixelFormat(u_int pixelFormat)
{
    switch (pixelFormat)
    {
      case MONO_8:
	return MONO_8;
      case YUV_411:
	return YUV_411;
      case YUV_422:
	return YUV_422;
      case YUV_444:
	return YUV_444;
      case RGB_24:
	return RGB_24;
      case MONO_16:
	return MONO_16;
      case RGB_48:
	return RGB_48;
      case SIGNED_MONO_16:
	return SIGNED_MONO_16;
      case SIGNED_RGB_48:
	return SIGNED_RGB_48;
      case RAW_8:
	return RAW_8;
      case RAW_16:
	return RAW_16;
    }

    throw std::invalid_argument("Unknown pixel format!!");
    
    return MONO_8;
}

//! 指定された画像フォーマットにおいてサポートされているフレームレートを調べる
/*!
  \param format	対象となるフォーマット
  \return	サポートされているフレームレートを #FrameRate 型の列挙値
		のorとして返す. 指定されたフォーマット自体がこのカメラでサ
		ポートされていなければ, 0が返される.
*/
quadlet_t
IIDCCamera::inquireFrameRate(Format format) const
{
    quadlet_t	quad = inquireFrameRate_or_Format_7_Offset(format);

    switch (format)
    {
      case Format_7_0:
      case Format_7_1:
      case Format_7_2:
      case Format_7_3:
      case Format_7_4:
      case Format_7_5:
      case Format_7_6:
      case Format_7_7:
	if (quad != 0)
	    return FrameRate_x;
	break;
      default:
	break;
    }

    return quad;
}

nodeaddr_t
IIDCCamera::getFormat_7_BaseAddr(Format format7) const
{
    if (format7 < Format_7_0)
	throw std::invalid_argument("IIDCCamera::getFormat_7_BaseAddr: not Format_7_x!!");

    const quadlet_t offset = inquireFrameRate_or_Format_7_Offset(format7) * 4;
    if (offset == 0)
	throw std::invalid_argument("IIDCCamera::getFormat_7_BaseAddr: unsupported Format_7_x!!");
    
    return CSR_REGISTER_BASE + offset;
}

u_int
IIDCCamera::setFormat_7_PacketSize(Format format7)
{
    const quadlet_t	Presence    = 0x1u << 31;
    const quadlet_t	Setting_1   = 0x1u << 30;
    const quadlet_t	ErrorFlag_1 = 0x1u << 23;
    const quadlet_t	ErrorFlag_2 = 0x1u << 22;
    const nodeaddr_t	base	= getFormat_7_BaseAddr(format7);
    const bool		present = _node->readQuadlet(base + VALUE_SETTING)
				& Presence;
    if (present)
    {
	_node->writeQuadlet(base + VALUE_SETTING, Setting_1);
	quadlet_t	quad;
	while ((quad = _node->readQuadlet(base + VALUE_SETTING)) & Setting_1)
	    ;
	if (quad & ErrorFlag_1)
	    throw std::runtime_error("IIDCCamera::setFormat_7_PacketSize: failed to read bytePerPacket value!!");
    }

    const u_int	recBytePerPacket = _node->readQuadlet(base + BYTE_PER_PACKET)
				 & 0xffff;
    if (recBytePerPacket != 0)	// 推奨値が有効ならば...
    {
      // 推奨値を設定
	_node->writeQuadlet(base + BYTE_PER_PACKET, recBytePerPacket << 16);

	if (!present ||
	    !(_node->readQuadlet(base + VALUE_SETTING) & ErrorFlag_2))
	    return recBytePerPacket;
    }

    const quadlet_t	quad = _node->readQuadlet(base + PACKET_PARA_INQ);
    const u_int		unitBytePerPacket = (quad & 0xffff0000) >> 16,
			maxBytePerPacket  = (quad & 0xffff);
    for (u_int n = (unitBytePerPacket != 0 ?
		    maxBytePerPacket / unitBytePerPacket : 0); n > 0; --n)
    {
	const u_int	bytePerPacket = n * unitBytePerPacket;
	_node->writeQuadlet(base + BYTE_PER_PACKET, bytePerPacket << 16);

	if (!present ||
	    !(_node->readQuadlet(base + VALUE_SETTING) & ErrorFlag_2))
	    return bytePerPacket;
    }

    throw std::runtime_error("IIDCCamera::setFormat_7_PacketSize: failed to set bytePerPacket!!");
    return 0;
}

quadlet_t
IIDCCamera::inquireFrameRate_or_Format_7_Offset(Format format) const
{
    const u_int		V_FORMAT_INQ	= 0x100;
    const quadlet_t	Format_0	= 0x1u << 31;
    const quadlet_t	Format_1	= 0x1u << 30;
    const quadlet_t	Format_2	= 0x1u << 29;
    const quadlet_t	Format_5	= 0x1u << 26;
    const quadlet_t	Format_6	= 0x1u << 25;
    const quadlet_t	Format_7	= 0x1u << 24;
    quadlet_t		inq		= 0;
    switch (format)	// Check presence of format.
    {
      case YUV444_160x120:
      case YUV422_320x240:
      case YUV411_640x480:
      case YUV422_640x480:
      case RGB24_640x480:
      case MONO8_640x480:
      case MONO16_640x480:
	inq = readQuadletFromRegister(V_FORMAT_INQ) & Format_0;
	break;
      case YUV422_800x600:
      case RGB24_800x600:
      case MONO8_800x600:
      case YUV422_1024x768:
      case RGB24_1024x768:
      case MONO8_1024x768:
      case MONO16_800x600:
      case MONO16_1024x768:
	inq = readQuadletFromRegister(V_FORMAT_INQ) & Format_1;
	break;
      case YUV422_1280x960:
      case RGB24_1280x960:
      case MONO8_1280x960:
      case YUV422_1600x1200:
      case RGB24_1600x1200:
      case MONO8_1600x1200:
      case MONO16_1280x960:
      case MONO16_1600x1200:
	inq = readQuadletFromRegister(V_FORMAT_INQ) & Format_2;
	break;
      case Format_5_0:
      case MONO8_640x480x2:
      case Format_5_2:
      case Format_5_3:
      case Format_5_4:
      case Format_5_5:
      case Format_5_6:
      case Format_5_7:
	inq = readQuadletFromRegister(V_FORMAT_INQ) & Format_5;
	break;
      case Format_7_0:
      case Format_7_1:
      case Format_7_2:
      case Format_7_3:
      case Format_7_4:
      case Format_7_5:
      case Format_7_6:
      case Format_7_7:
	inq = readQuadletFromRegister(V_FORMAT_INQ) & Format_7;
	break;
    }
    if (inq == 0)
	return 0;

    const u_int		V_MODE_INQ_0	= 0x180;
    const u_int		V_MODE_INQ_1	= 0x184;
    const u_int		V_MODE_INQ_2	= 0x188;
    const u_int		V_MODE_INQ_5	= 0x194;
    const u_int		V_MODE_INQ_6	= 0x198;
    const u_int		V_MODE_INQ_7	= 0x19c;
    const quadlet_t	Mode_0		= 0x1u << 31;
    const quadlet_t	Mode_1		= 0x1u << 30;
    const quadlet_t	Mode_2		= 0x1u << 29;
    const quadlet_t	Mode_3		= 0x1u << 28;
    const quadlet_t	Mode_4		= 0x1u << 27;
    const quadlet_t	Mode_5		= 0x1u << 26;
    const quadlet_t	Mode_6		= 0x1u << 25;
    const quadlet_t	Mode_7		= 0x1u << 26;
    inq = 0;
    switch (format)	// Check presence of mode.
    {
      case YUV444_160x120:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_0;
	break;
      case YUV422_320x240:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_1;
	break;
      case YUV411_640x480:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_2;
	break;
      case YUV422_640x480:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_3;
	break;
      case RGB24_640x480:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_4;
	break;
      case MONO8_640x480:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_5;
	break;
      case MONO16_640x480:
	inq = readQuadletFromRegister(V_MODE_INQ_0) & Mode_6;
	break;
      case YUV422_800x600:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_0;
	break;
      case RGB24_800x600:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_1;
	break;
      case MONO8_800x600:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_2;
	break;
      case YUV422_1024x768:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_3;
	break;
      case RGB24_1024x768:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_4;
	break;
      case MONO8_1024x768:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_5;
	break;
      case MONO16_800x600:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_6;
	break;
      case MONO16_1024x768:
	inq = readQuadletFromRegister(V_MODE_INQ_1) & Mode_7;
	break;
      case YUV422_1280x960:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_0;
	break;
      case RGB24_1280x960:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_1;
	break;
      case MONO8_1280x960:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_2;
	break;
      case YUV422_1600x1200:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_3;
	break;
      case RGB24_1600x1200:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_4;
	break;
      case MONO8_1600x1200:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_5;
	break;
      case MONO16_1280x960:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_6;
	break;
      case MONO16_1600x1200:
	inq = readQuadletFromRegister(V_MODE_INQ_2) & Mode_7;
	break;
      case Format_5_0:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_0;
	break;
      case MONO8_640x480x2:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_1;
	break;
      case Format_5_2:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_2;
	break;
      case Format_5_3:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_3;
	break;
      case Format_5_4:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_4;
	break;
      case Format_5_5:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_5;
	break;
      case Format_5_6:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_6;
	break;
      case Format_5_7:
	inq = readQuadletFromRegister(V_MODE_INQ_5) & Mode_7;
	break;
      case Format_7_0:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_0;
	break;
      case Format_7_1:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_1;
	break;
      case Format_7_2:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_2;
	break;
      case Format_7_3:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_3;
	break;
      case Format_7_4:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_4;
	break;
      case Format_7_5:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_5;
	break;
      case Format_7_6:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_6;
	break;
      case Format_7_7:
	inq = readQuadletFromRegister(V_MODE_INQ_7) & Mode_7;
	break;
    }
    if (inq == 0)
	return 0;
    
    return readQuadletFromRegister(format);
}

//! カメラベンダー依存の機能へのアクセス制限を解除してそれを使えるようにする
/*
  \param featureId	アクセス制限を解除したい機能を表す48bitのID. 
  \param timeout	解除してからまたロックされるまでのタイムアウト値
			(単位: msec)
  \return		解除に成功すればtrueを, 失敗するかこの機能自体が
			存在しなければfalseを返す.
*/
bool
IIDCCamera::unlockAdvancedFeature(uint64_t featureId, u_int timeout)
{
    if (_acRegBase == 0)
	return false;
    _node->writeQuadlet(_acRegBase, featureId >> 16);	// upper 32bits
    _node->writeQuadlet(_acRegBase + 4,
			(featureId << 16) | 0xf000 | (timeout & 0xfff));
    u_int	busId_nodeId = _node->readQuadlet(_acRegBase) >> 16;

    return _node->nodeId() == busId_nodeId;
}

/************************************************************************
*  global functions							*
************************************************************************/
static const struct
{
    IIDCCamera::Format	format;
    const char*			name;
} formats[] =
{
    {IIDCCamera::YUV444_160x120,	"160x120-YUV(4:4:4)"},
    {IIDCCamera::YUV422_320x240,	"320x240-YUV(4:2:2)"},
    {IIDCCamera::YUV411_640x480,	"640x480-YUV(4:1:1)"},
    {IIDCCamera::YUV422_640x480,	"640x480-YUV(4:2:2)"},
    {IIDCCamera::RGB24_640x480,		"640x480-RGB"},
    {IIDCCamera::MONO8_640x480,		"640x480-Y(mono)"},
    {IIDCCamera::MONO16_640x480,	"640x480-Y(mono16)"},
    {IIDCCamera::YUV422_800x600,	"800x600-YUV(4:2:2)"},
    {IIDCCamera::RGB24_800x600,		"800x600-RGB"},
    {IIDCCamera::MONO8_800x600,		"800x600-Y(mono)"},
    {IIDCCamera::YUV422_1024x768,	"1024x768-YUV(4:2:2)"},
    {IIDCCamera::RGB24_1024x768,	"1024x768-RGB"},
    {IIDCCamera::MONO8_1024x768,	"1024x768-Y(mono)"},
    {IIDCCamera::MONO16_800x600,	"800x600-Y(mono16)"},
    {IIDCCamera::MONO16_1024x768,	"1024x768-Y(mono16)"},
    {IIDCCamera::YUV422_1280x960,	"1280x960-YUV(4:2:2)"},
    {IIDCCamera::RGB24_1280x960,	"1280x960-RGB"},
    {IIDCCamera::MONO8_1280x960,	"1280x960-Y(mono)"},
    {IIDCCamera::YUV422_1600x1200,	"1600x1200-YUV(4:2:2)"},
    {IIDCCamera::RGB24_1600x1200,	"1600x1200-RGB"},
    {IIDCCamera::MONO8_1600x1200,	"1600x1200-Y(mono)"},
    {IIDCCamera::MONO16_1280x960,	"1280x960-Y(mono16)"},
    {IIDCCamera::MONO16_1600x1200,	"1600x1200-Y(mono16)"},
    {IIDCCamera::Format_5_0,		"Format_5_0"},
    {IIDCCamera::MONO8_640x480x2,	"640x480x2-Y(mono)"},
    {IIDCCamera::Format_5_2,		"Format_5_2"},
    {IIDCCamera::Format_5_3,		"Format_5_3"},
    {IIDCCamera::Format_5_4,		"Format_5_4"},
    {IIDCCamera::Format_5_5,		"Format_5_5"},
    {IIDCCamera::Format_5_6,		"Format_5_6"},
    {IIDCCamera::Format_5_7,		"Format_5_7"},
    {IIDCCamera::Format_7_0,		"Format_7_0"},
    {IIDCCamera::Format_7_1,		"Format_7_1"},
    {IIDCCamera::Format_7_2,		"Format_7_2"},
    {IIDCCamera::Format_7_3,		"Format_7_3"},
    {IIDCCamera::Format_7_4,		"Format_7_4"},
    {IIDCCamera::Format_7_5,		"Format_7_5"},
    {IIDCCamera::Format_7_6,		"Format_7_6"},
    {IIDCCamera::Format_7_7,		"Format_7_7"}
};

static const struct
{
    IIDCCamera::FrameRate	rate;
    const char*			name;
} rates[] =
{
    {IIDCCamera::FrameRate_1_875,	"1.875fps"},
    {IIDCCamera::FrameRate_3_75,	"3.75fps"},
    {IIDCCamera::FrameRate_7_5,		"7.5fps"},
    {IIDCCamera::FrameRate_15,		"15fps"},
    {IIDCCamera::FrameRate_30,		"30fps"},
    {IIDCCamera::FrameRate_60,		"60fps"},
    {IIDCCamera::FrameRate_120,		"120fps"},
    {IIDCCamera::FrameRate_240,		"240fps"},
    {IIDCCamera::FrameRate_x,		"custom_frame_rate"}
};

static const struct
{
    IIDCCamera::Feature	feature;
    const char*			name;
} features[] =
{
    {IIDCCamera::BRIGHTNESS,		"BRIGHTNESS"},
    {IIDCCamera::AUTO_EXPOSURE,		"AUTO_EXPOSURE"},
    {IIDCCamera::SHARPNESS,		"SHARPNESS"},
    {IIDCCamera::WHITE_BALANCE,		"WHITE_BALANCE"},
    {IIDCCamera::HUE,			"HUE"},
    {IIDCCamera::SATURATION,		"SATURATION"},
    {IIDCCamera::GAMMA,			"GAMMA"},
    {IIDCCamera::SHUTTER,		"SHUTTER"},
    {IIDCCamera::GAIN,			"GAIN"},
    {IIDCCamera::IRIS,			"IRIS"},
    {IIDCCamera::FOCUS,			"FOCUS"},
    {IIDCCamera::TEMPERATURE,		"TEMPERATURE"},
    {IIDCCamera::ZOOM,			"ZOOM"},
    {IIDCCamera::PAN,			"PAN"},
    {IIDCCamera::TILT,			"TILT"}
};

/************************************************************************
*  global functions							*
************************************************************************/
//! 現在のカメラの設定をストリームに書き出す
/*!
  \param out		出力ストリーム
  \param camera		対象となるカメラ
  \return		outで指定した出力ストリーム
*/
std::ostream&
operator <<(std::ostream& out, const IIDCCamera& camera)
{
    using namespace	std;
    
    for (const auto& format : formats)
	if (camera.getFormat() == format.format)
	{
	    out << format.name;
	    break;
	}

    for (const auto& rate : rates)
	if (camera.getFrameRate() == rate.rate)
	{
	    out << ' ' << rate.name;
	    break;
	}

    for (const auto& feature : features)
    {
	u_int	inq = camera.inquireFeatureFunction(feature.feature);

	if ((inq & IIDCCamera::Presence) &&
	    (inq & IIDCCamera::Manual)   &&
	    (inq & IIDCCamera::ReadOut))
	{
	    out << ' ' << feature.name;
	    
	    if ((inq & IIDCCamera::Auto) &&
		camera.isAuto(feature.feature))
		out << ' ' << -1;
	    else
	    {
		if (feature.feature == IIDCCamera::WHITE_BALANCE)
		{
		    u_int	ub, vr;
		    camera.getWhiteBalance(ub, vr);
		    out << ' ' << ub << ' ' << vr;
		}
		else
		    out << ' ' << camera.getValue(feature.feature);
	    }
	}
    }

    return out << endl;
}

//! ストリームから読み込んだ設定をカメラにセットする
/*!
  \param in		入力ストリーム
  \param camera		対象となるカメラ
  \return		inで指定した入力ストリーム
*/
std::istream&
operator >>(std::istream& in, IIDCCamera& camera)
{
    using namespace	std;

  // Read format.
    string	s;
    in >> s;
    auto	format = std::find_if(std::begin(formats), std::end(formats),
				      [&](decltype(formats[0]) fmt)
				      {
					  return s == fmt.name;
				      });

  // Read frame rate.
    in >> s;
    auto	rate = std::find_if(std::begin(rates), std::end(rates),
				    [&](decltype(rates[0]) rt)
				    {
					return s == rt.name;
				    });

  // Read features.
    for (char c; in.get(c) && c != '\n'; )
    {
	in.putback(c);
	int	val, val2;
	in >> s >> val;
	if (s == "WHITE_BALANCE" && val != -1)
	    in >> val2;
	
	for (const auto& feature : features)
	    if (s == feature.name)
	    {
		u_int inq = camera.inquireFeatureFunction(feature.feature);

		if (inq & IIDCCamera::Presence)
		{
		    if (val == -1)
		    {
			if (inq & IIDCCamera::Auto)
			    camera.setAutoMode(feature.feature);
		    }
		    else
		    {
			if (inq & IIDCCamera::Manual)
			{
			    camera.setManualMode(feature.feature);
			    if (feature.feature == IIDCCamera::WHITE_BALANCE)
				camera.setWhiteBalance(val, val2);
			    else
				camera.setValue(feature.feature, val);
			}
		    }
		}
	    }
    }
    
    if (format != std::end(formats) && rate != std::end(rates))
	camera.setFormatAndFrameRate(format->format, rate->rate);
        
    return in;
}
 
#ifdef HAVE_LIBTUTOOLS__
template const IIDCCamera&
IIDCCamera::operator >>(Image<u_char>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<short>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<float>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<double>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<RGB>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<RGBA>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<BGR>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<ABGR>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<YUV444>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<YUV422>& image)	const	;
template const IIDCCamera&
IIDCCamera::operator >>(Image<YUV411>& image)	const	;
template const IIDCCamera&
IIDCCamera::captureRGBImage(Image<RGB>& image)	const	;
template const IIDCCamera&
IIDCCamera::captureRGBImage(Image<RGBA>& image)	const	;
template const IIDCCamera&
IIDCCamera::captureRGBImage(Image<BGR>& image)	const	;
template const IIDCCamera&
IIDCCamera::captureRGBImage(Image<ABGR>& image)	const	;
#endif		// HAVE_LIBTUTOOLS__
}
/*
 *  $Id$
 */
#ifndef TU_V_VIIDCPP_H
#define TU_V_VIIDCPP_H

#include "TU/v/CmdPane.h"
#include "TU/IIDCCameraArray.h"

namespace TU
{
namespace v
{
/************************************************************************
*  global functions							*
************************************************************************/
MenuDef*	createFormatMenu(const IIDCCamera& camera)		;
IIDCCamera::PixelFormat
		selectROI(IIDCCamera& camera,
			  IIDCCamera::Format format7,
			  size_t& u0, size_t& v0, size_t& width,
			  size_t& height, size_t& packetSize,
			  Window& window)				;
CmdDef*		createFeatureCmds(const IIDCCamera& camera,
				  size_t ncameras=1)			;
void		refreshFeatureCmds(const IIDCCamera& camera,
				   CmdPane& cmdPane)			;
void		refreshSliderCmd(const IIDCCamera& camera,
				 CmdId id, CmdPane& cmdPane)		;
    
template <class CAMERAS> auto
setFormat(CAMERAS&& cameras, CmdId id, CmdVal val, Window& window)
    -> typename std::enable_if<
	  std::is_convertible<
	      typename std::remove_reference<
		  decltype(*std::begin(cameras))>::type, IIDCCamera>::value,
	      bool>::type
{
    switch (id)
    {
      case IIDCCamera::Format_7_0:
      case IIDCCamera::Format_7_1:
      case IIDCCamera::Format_7_2:
      case IIDCCamera::Format_7_3:
      case IIDCCamera::Format_7_4:
      case IIDCCamera::Format_7_5:
      case IIDCCamera::Format_7_6:
      case IIDCCamera::Format_7_7:
	if (std::size(cameras) != 0)
	{
	    const auto	format7 = IIDCCamera::uintToFormat(id);
	    size_t	u0, v0, width, height, packetSize;
	    const auto	pixelFormat = selectROI(*std::begin(cameras), format7,
						u0, v0, width, height,
						packetSize, window);
	    for (auto& camera : cameras)
		camera.setFormat_7_ROI(format7, u0, v0, width, height)
		      .setFormat_7_PixelFormat(format7, pixelFormat)
		      .setFormat_7_PacketSize(format7, packetSize);
	}
        break;

      default:
	break;
    }
    
    return setFormat(cameras, id, val);
}

inline bool
setFormat(IIDCCamera& camera, CmdId id, CmdVal val, Window& window)
{
    return setFormat(make_range(&camera, 1), id, val, window);
}

template <class CAMERAS> auto
refreshFeatureCmds(const CAMERAS& cameras, CmdPane& cmdPane)
    -> typename std::enable_if<std::is_convertible<
				   decltype(*std::begin(cameras)),
				   const IIDCCamera&>::value>::type
{
    auto	camera = std::begin(cameras);
    
    switch (std::size(cameras))
    {
      case 0:
	return;
      case 1:
	break;
      default:
	std::advance(camera, cmdPane.getValue(IIDCCAMERA_CHOICE));
	break;
    }
    
    refreshFeatureCmds(*camera, cmdPane);
}
    
template <class CAMERAS> auto
setFeature(CAMERAS&& cameras, CmdId id, CmdVal val, CmdPane& cmdPane)
    -> typename std::enable_if<
	  std::is_convertible<
	      typename std::remove_reference<
		  decltype(*std::begin(cameras))>::type, IIDCCamera>::value,
	      bool>::type
{
    auto	camera = std::begin(cameras);
    
    if (id == IIDCCAMERA_CHOICE)		// 選択カメラが変更されたら...
    {
	std::advance(camera, val);
	refreshFeatureCmds(*camera, cmdPane);	// カメラの全属性をGUIに反映
	return true;
    }

    if (std::size(cameras) > 1 &&		// カメラが複数かつ
	!cmdPane.getValue(IIDCCAMERA_ALL))	// 全カメラ操作モードでなければ...
    {
	std::advance(camera, cmdPane.getValue(IIDCCAMERA_CHOICE));
	if (!setFeature(*camera, id, int(val), val.f()))   // 選択カメラの属性設定
	    return false;
    }
    else if (!setFeature(cameras, id, int(val), val.f()))  // 全カメラの属性設定
	return false;
    
  // idが属性の絶対値指定でなければGUIを更新する必要はない
    if (camera == std::end(cameras) ||
	u_int(id) < IIDCCamera::BRIGHTNESS + IIDCCAMERA_OFFSET_ABS)
	return true;
    
  // 属性の絶対値指定が変更されたら、sliderコマンドの定義域と値を更新する
    refreshSliderCmd(*camera, id - IIDCCAMERA_OFFSET_ABS, cmdPane);
	
    return true;
}

inline bool
setFeature(IIDCCamera& camera, CmdId id, CmdVal val, CmdPane& cmdPane)
{
    return setFeature(make_range(&camera, 1), id, val, cmdPane);
}

}	// namespace v
}	// namespace TU
#endif	// !TU_V_IIDCPP_H

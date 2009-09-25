/*
 *  $Id: MyModalDialog.h,v 1.1 2009-07-28 00:00:48 ueshiba Exp $
 */
#include "TU/Ieee1394++.h"
#include "TU/v/ModalDialog.h"

namespace TU
{
namespace v
{
/************************************************************************
*  class MyModalDialog							*
************************************************************************/
class MyModalDialog : public ModalDialog
{
  public:
    MyModalDialog(Window& parentWindow,
		  const Ieee1394Camera::Format_7_Info& fmt7info)	;
    
    Ieee1394Camera::PixelFormat	getROI(u_int& u0, u_int& v0,
				       u_int& width, u_int& height)	;
    virtual void		callback(CmdId id, CmdVal val)		;

  private:
    const Ieee1394Camera::Format_7_Info&	_fmt7info;
};
    
}
}
#include "pch.h"
#include "SaveImage.h"
#include "ImageSaver.h"

//--------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する。
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//--------------------------------------------------------------------

AviUtlInternal g_auin;
AviUtl::FilterPlugin* g_exedit = 0;

//--------------------------------------------------------------------

BOOL onCommand(int commandIndex, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	MY_TRACE(_T("onCommand(%d)\n"), commandIndex);

	switch (commandIndex)
	{
	case Check::SaveFrameRGB:
	case Check::SaveFrameRGBA:
	case Check::SaveItemRGB:
	case Check::SaveItemRGBA:
		{
			ImageSaver imageSaver(editp, fp, commandIndex);

			return imageSaver.main();
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------

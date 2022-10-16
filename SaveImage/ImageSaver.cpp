#include "pch.h"
#include "ImageSaver.h"
#include "SaveImage.h"

//--------------------------------------------------------------------

using func_proc_type = decltype(AviUtl::FilterPlugin::func_proc);
func_proc_type true_exedit_func_proc = 0;
BOOL hook_exedit_func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
{
	MY_TRACE(_T("hook_exedit_func_proc() begin\n"));

	const AviUtl::PixelYC white = { 4096, 0, 0 };
	AviUtl::PixelYC* ycp_edit = (AviUtl::PixelYC*)fpip->ycp_edit;

	for (int y = 0; y < fpip->h; y++)
	{
		int lineBegin = y * fpip->max_w;

		for (int x = 0; x < fpip->w; x++)
		{
			ycp_edit[lineBegin + x] = white;
		}
	}

	BOOL result = true_exedit_func_proc(fp, fpip);

	MY_TRACE(_T("hook_exedit_func_proc() end\n"));

	return result;
}

//--------------------------------------------------------------------

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

//--------------------------------------------------------------------

ImageSaver::ImageSaver(AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp, int command)
{
	m_editp = editp;
	m_fp = fp;
	m_command = command;
}

//--------------------------------------------------------------------

BOOL ImageSaver::main()
{
	MY_TRACE(_T("ImageSaver::main(%d)\n"), m_command);

	if (!m_fp->exfunc->is_editing(m_editp)) return FALSE;

	char fileName[MAX_PATH] = {};
	if (!m_fp->exfunc->dlg_get_save_name(fileName, "PNG / BMP / JPG / GIF / TIFF File\0*.png;*.bmp;*.jpg;*.gif;*.tiff\0All File (*.*)\0*.*\0", 0))
		return FALSE;

	LPCSTR extension = ::PathFindExtensionA(fileName);
	MY_TRACE_STR(extension);

	ULONG m_quality = m_fp->track[Track::Quality];
	EncoderParameters encoderParameters = {};
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &m_quality;

	CLSID encoder = {};
	int result = -1;
	if (::lstrcmpiA(extension, ".bmp") == 0) result = GetEncoderClsid(L"image/bmp", &encoder);
	else if (::lstrcmpiA(extension, ".jpg") == 0) result = GetEncoderClsid(L"image/jpeg", &encoder);
	else if (::lstrcmpiA(extension, ".gif") == 0) result = GetEncoderClsid(L"image/gif", &encoder);
	else if (::lstrcmpiA(extension, ".tif") == 0) result = GetEncoderClsid(L"image/tiff", &encoder);
	else if (::lstrcmpiA(extension, ".png") == 0) result = GetEncoderClsid(L"image/png", &encoder);

	if (result == -1)
	{
		::MessageBox(0, _T("拡張子が無効です"), _T("SaveImage"), MB_OK);

		return FALSE;
	}

	DWORD start = ::timeGetTime();

	int frame = m_fp->exfunc->get_frame(m_editp);
	int width = 0, height = 0;
	m_fp->exfunc->get_pixel_filtered(m_editp, frame, 0, &width, &height);

	std::vector<AviUtl::PixelBGR> src1;
	std::vector<AviUtl::PixelBGR> src2;
	std::vector<ColorRGBA> src;

	// 選択オブジェクトを囲う点線を消すために func_save_start を呼ぶ
	if (g_exedit) g_exedit->func_save_start(g_exedit, frame, frame, m_editp);

	src2.resize(width * height);
	m_fp->exfunc->get_pixel_filtered(m_editp, frame, src2.data(), 0, 0);

	if (g_exedit &&
		(m_command == Check::SaveFrameRGBA ||
		m_command == Check::SaveItemRGBA))
	{
		// func_proc をすり替え
		true_exedit_func_proc = g_exedit->func_proc;
		g_exedit->func_proc = hook_exedit_func_proc;

		src1.resize(width * height);
		m_fp->exfunc->get_pixel_filtered(m_editp, frame, src1.data(), 0, 0);

		// func_proc を元に戻す
		g_exedit->func_proc = true_exedit_func_proc;

		src.resize(src2.size());

		for (int y = 0; y < height; y++)
		{
			int ii = y * width;
			int jj = (height - y - 1) * width;

			for (int x = 0; x < width; x++)
			{
				int i = ii + x;
				int j = jj + x;

				BYTE ra = (BYTE)(255 - src1[i].r + src2[i].r);
				BYTE ga = (BYTE)(255 - src1[i].g + src2[i].g);
				BYTE ba = (BYTE)(255 - src1[i].b + src2[i].b);
				src[j].a = (BYTE)((ra+ga+ba)/3);
				src[j].r = ra ? (BYTE)(src2[i].r * 255 / ra) : src2[i].r;
				src[j].g = ga ? (BYTE)(src2[i].g * 255 / ga) : src2[i].g;
				src[j].b = ba ? (BYTE)(src2[i].b * 255 / ba) : src2[i].b;
			}
		}
	}
	else
	{
		src.resize(src2.size());

		for (int y = 0; y < height; y++)
		{
			int ii = y * width;
			int jj = (height - y - 1) * width;

			for (int x = 0; x < width; x++)
			{
				int i = ii + x;
				int j = jj + x;

				src[j].a = 0x00;
				src[j].r = src2[i].r;
				src[j].g = src2[i].g;
				src[j].b = src2[i].b;
			}
		}
	}

	if (g_exedit) g_exedit->func_save_end(g_exedit, m_editp);

	if (m_command == Check::SaveItemRGB ||
		m_command == Check::SaveItemRGBA)
	{
		BYTE* positionData = g_auin.GetPositionDataArray();
		int pd_x = *(int*)(positionData + 0x1C);
		int pd_y = *(int*)(positionData + 0x20);
		int pd_w = *(int*)(positionData + 0x24);
		int pd_h = *(int*)(positionData + 0x28);

		if (pd_x < 0 || pd_w <= 0 || (pd_x + pd_w) > width)
		{
			::MessageBox(0, _T("アイテムの位置情報が無効です"), _T("SaveImage"), MB_OK);

			return FALSE;
		}

		if (pd_y < 0 || pd_h <= 0 || (pd_y + pd_h) > height)
		{
			::MessageBox(0, _T("アイテムの位置情報が無効です"), _T("SaveImage"), MB_OK);

			return FALSE;
		}

		ColorRGBA* dstHead = src.data();
		ColorRGBA* srcHead = src.data() + pd_y * width + pd_x;

		for (int y = 0; y < pd_h; y++)
		{
			ColorRGBA* dstLine = dstHead + y * pd_w;
			ColorRGBA* srcLine = srcHead + y * width;

			for (int x = 0; x < pd_w; x++)
			{
				dstLine[x] = srcLine[x];
			}
		}

		width = pd_w;
		height = pd_h;
	}

	std::shared_ptr<Bitmap> bitmap;

	if (m_command == Check::SaveFrameRGB ||
		m_command == Check::SaveItemRGB)
	{
		BITMAPINFO bi = {};
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth = width;
		bi.bmiHeader.biHeight = -height;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;

		bitmap = std::make_shared<Bitmap>(&bi, src.data());
	}
	else
	{
		bitmap = std::make_shared<Bitmap>(width, height,
			width * sizeof(ColorRGBA), PixelFormat32bppARGB, (BYTE*)src.data());
	}

	Status status;
	if (::lstrcmpiA(extension, ".jpg") == 0)
		status = bitmap->Save((_bstr_t)fileName, &encoder, &encoderParameters);
	else
		status = bitmap->Save((_bstr_t)fileName, &encoder);
	MY_TRACE_COM_ERROR(status);

	if (status != S_OK)
	{
		::MessageBox(0, _T("ファイルの保存に失敗しました"), _T("SaveImage"), MB_OK);

		return FALSE;
	}

	DWORD end = ::timeGetTime();

	MY_TRACE(_T("ImageSaver::main() => %dms.\n"), end - start);

	return TRUE;
}

//--------------------------------------------------------------------

#pragma once

inline struct App
{
	struct Track
	{
		static const int32_t Quality = 0;
	};

	struct Check
	{
		static const int32_t SaveFrameRGB = 0;
		static const int32_t SaveFrameRGBA = 1;
		static const int32_t SaveItemRGB = 2;
		static const int32_t SaveItemRGBA = 3;
	};

	AviUtlInternal auin;
	AviUtl::FilterPlugin* exedit = 0;

	BOOL onInit(AviUtl::FilterPlugin* fp)
	{
		auin.initExEditAddress();
		exedit = auin.GetFilter(fp, "拡張編集");

		fp->exfunc->add_menu_item(fp, "現在フレーム画像を保存", fp->hwnd, Check::SaveFrameRGB, 0, AviUtl::ExFunc::AddMenuItemFlag::None);
		fp->exfunc->add_menu_item(fp, "現在フレーム画像をアルファ付きで保存", fp->hwnd, Check::SaveFrameRGBA, 0, AviUtl::ExFunc::AddMenuItemFlag::None);
		fp->exfunc->add_menu_item(fp, "選択アイテム画像を保存", fp->hwnd, Check::SaveItemRGB, 0, AviUtl::ExFunc::AddMenuItemFlag::None);
		fp->exfunc->add_menu_item(fp, "選択アイテム画像をアルファ付きで保存", fp->hwnd, Check::SaveItemRGBA, 0, AviUtl::ExFunc::AddMenuItemFlag::None);

		return TRUE;
	}

	BOOL onCommand(int index, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
	{
		MY_TRACE(_T("onCommand(%d)\n"), index);

		switch (index)
		{
		case Check::SaveFrameRGB:
		case Check::SaveFrameRGBA:
		case Check::SaveItemRGB:
		case Check::SaveItemRGBA:
			{
				BOOL hasAlpha = (index == Check::SaveFrameRGBA || index == Check::SaveItemRGBA);
				BOOL itemOnly = (index == Check::SaveItemRGB || index == Check::SaveItemRGBA);

				saveImage(editp, fp, hasAlpha, itemOnly);

				break;
			}
		}

		return FALSE;
	}

#pragma pack(push)
#pragma pack(1)
	struct PixelARGB { uint8_t b, g, r, a; };
#pragma pack(pop)

	template<class T>
	struct Bits
	{
		int32_t width, height, stride;
		std::unique_ptr<uint8_t[]> bits;

		Bits(int32_t width, int32_t height, int32_t stride)
			: width(width), height(height), stride(stride)
			, bits(std::make_unique<uint8_t[]>(height * stride))
		{
		}

		T* get()
		{
			return reinterpret_cast<T*>(bits.get());
		}

		T* get_line(int32_t y)
		{
			return reinterpret_cast<T*>(bits.get() + y * stride);
		}
	};

	inline static decltype(AviUtl::FilterPlugin::func_proc) true_exedit_func_proc = 0;
	static BOOL hook_exedit_func_proc(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip)
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

	static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
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

	BOOL saveImage(AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp, BOOL hasAlpha, BOOL itemOnly)
	{
		MY_TRACE(_T("saveImage()\n"));

		if (!fp->exfunc->is_editing(editp)) return FALSE;

		char fileName[MAX_PATH] = {};
		if (!fp->exfunc->dlg_get_save_name(fileName, "PNG / BMP / JPG / GIF / TIFF File\0*.png;*.bmp;*.jpg;*.gif;*.tiff\0All File (*.*)\0*.*\0", 0))
			return FALSE;

		LPCSTR extension = ::PathFindExtensionA(fileName);
		MY_TRACE_STR(extension);

		ULONG quality = fp->track[Track::Quality];
		EncoderParameters encoderParameters = {};
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &quality;

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

		// フレームの映像サイズを取得します。
		int32_t frame = fp->exfunc->get_frame(editp);
		int32_t width = 0, height = 0;
		fp->exfunc->get_pixel_filtered(editp, frame, 0, &width, &height);
		int32_t stride = width * 3 + width % 4;

		// 出力バッファを確保します。
		Bits<PixelARGB> output(width, height, width * 4);

		// 選択オブジェクトを囲う点線を消すために func_save_start を呼ぶ
		if (exedit) exedit->func_save_start(exedit, frame, frame, editp);

		// 入力バッファを確保します。
		Bits<AviUtl::PixelBGR> input(width, height, stride);
		fp->exfunc->get_pixel_filtered(editp, frame, input.get(), 0, 0);

		if (exedit && hasAlpha)
		{
			// func_proc をすり替え
			true_exedit_func_proc = exedit->func_proc;
			exedit->func_proc = hook_exedit_func_proc;

			// アルファ算出用のサブ入力バッファを確保します。
			Bits<AviUtl::PixelBGR> sub_input(width, height, stride);
			fp->exfunc->get_pixel_filtered(editp, frame, sub_input.get(), 0, 0);

			// func_proc を元に戻す
			exedit->func_proc = true_exedit_func_proc;

			for (int y = 0; y < height; y++)
			{
				auto output_line = output.get_line(height - y - 1);
				auto input_line = input.get_line(y);
				auto sub_input_line = sub_input.get_line(y);

				for (int x = 0; x < width; x++)
				{
					BYTE ra = (BYTE)(255 - sub_input_line[x].r + input_line[x].r);
					BYTE ga = (BYTE)(255 - sub_input_line[x].g + input_line[x].g);
					BYTE ba = (BYTE)(255 - sub_input_line[x].b + input_line[x].b);
					output_line[x].a = (BYTE)((ra+ga+ba)/3);
					output_line[x].r = ra ? (BYTE)(input_line[x].r * 255 / ra) : input_line[x].r;
					output_line[x].g = ga ? (BYTE)(input_line[x].g * 255 / ga) : input_line[x].g;
					output_line[x].b = ba ? (BYTE)(input_line[x].b * 255 / ba) : input_line[x].b;
				}
			}
		}
		else
		{
			for (int y = 0; y < height; y++)
			{
				auto output_line = output.get_line(height - y - 1);
				auto input_line = input.get_line(y);

				for (int x = 0; x < width; x++)
				{
					output_line[x].a = 255;
					output_line[x].r = input_line[x].r;
					output_line[x].g = input_line[x].g;
					output_line[x].b = input_line[x].b;
				}
			}
		}

		if (exedit) exedit->func_save_end(exedit, editp);

		if (itemOnly)
		{
			BYTE* positionData = auin.GetPositionDataArray();
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

			PixelARGB* dstHead = output.get();
			PixelARGB* srcHead = output.get() + pd_y * width + pd_x;

			for (int y = 0; y < pd_h; y++)
			{
				PixelARGB* dstLine = dstHead + y * pd_w;
				PixelARGB* srcLine = srcHead + y * width;

				for (int x = 0; x < pd_w; x++)
				{
					dstLine[x] = srcLine[x];
				}
			}

			width = pd_w;
			height = pd_h;
		}

		Bitmap bitmap(width, height, width * 4, PixelFormat32bppARGB, (BYTE*)output.get());

		Status status;
		if (::lstrcmpiA(extension, ".jpg") == 0)
			status = bitmap.Save((_bstr_t)fileName, &encoder, &encoderParameters);
		else
			status = bitmap.Save((_bstr_t)fileName, &encoder);
		MY_TRACE_COM_ERROR(status);

		if (status != S_OK)
		{
			::MessageBox(0, _T("ファイルの保存に失敗しました"), _T("SaveImage"), MB_OK);

			return FALSE;
		}

		DWORD end = ::timeGetTime();

		MY_TRACE(_T("saveImage() => %dms.\n"), end - start);

		return TRUE;
	}
} app;

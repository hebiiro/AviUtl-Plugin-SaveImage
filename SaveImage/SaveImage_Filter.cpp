#include "pch.h"
#include "SaveImage.h"

//--------------------------------------------------------------------

BOOL func_init(AviUtl::FilterPlugin* fp)
{
	MY_TRACE(_T("func_init()\n"));

	g_auin.initExEditAddress();
	g_exedit = g_auin.GetFilter(fp, "拡張編集");

	fp->exfunc->add_menu_item(fp, "現在フレーム画像を保存", fp->hwnd, Check::SaveFrameRGB, 0, AviUtl::ExFunc::AddMenuItemFlag::None);
	fp->exfunc->add_menu_item(fp, "現在フレーム画像をアルファ付きで保存", fp->hwnd, Check::SaveFrameRGBA, 0, AviUtl::ExFunc::AddMenuItemFlag::None);
	fp->exfunc->add_menu_item(fp, "選択アイテム画像を保存", fp->hwnd, Check::SaveItemRGB, 0, AviUtl::ExFunc::AddMenuItemFlag::None);
	fp->exfunc->add_menu_item(fp, "選択アイテム画像をアルファ付きで保存", fp->hwnd, Check::SaveItemRGBA, 0, AviUtl::ExFunc::AddMenuItemFlag::None);

	return TRUE;
}

BOOL func_exit(AviUtl::FilterPlugin* fp)
{
	MY_TRACE(_T("func_exit()\n"));

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
//	MY_TRACE(_T("func_WndProc(0x%08X, 0x%08X, 0x%08X)\n"), message, wParam, lParam);

	switch (message)
	{
	case AviUtl::FilterPlugin::WindowMessage::Init:
		{
			MY_TRACE(_T("func_WndProc(Init, 0x%08X, 0x%08X)\n"), wParam, lParam);

			break;
		}
	case AviUtl::FilterPlugin::WindowMessage::Exit:
		{
			MY_TRACE(_T("func_WndProc(Exit, 0x%08X, 0x%08X)\n"), wParam, lParam);

			break;
		}
	case AviUtl::FilterPlugin::WindowMessage::Command:
		{
			MY_TRACE(_T("func_WndProc(Command, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (wParam == 0 && lParam == 0) return TRUE;

			return onCommand(LOWORD(wParam), editp, fp);
		}
	case WM_COMMAND:
		{
			return onCommand(LOWORD(wParam) - fp->MidFilterButton, editp, fp);
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------

LPCSTR track_name[] =
{
	"品質",
};

int track_def[] = {  90 };
int track_min[] = {   0 };
int track_max[] = { 100 };

LPCSTR check_name[] =
{
	"現在フレーム画像を保存",
	"現在フレーム画像をアルファ付きで保存",
	"選択アイテム画像を保存",
	"選択アイテム画像をアルファ付きで保存",
};

int check_def[] = { -1, -1, -1, -1 };

EXTERN_C AviUtl::FilterPluginDLL* CALLBACK GetFilterTable()
{
	LPCSTR name = "画像保存";
	LPCSTR information = "画像保存 1.0.0 by 蛇色";

	static AviUtl::FilterPluginDLL filter =
	{
		.flag =
			AviUtl::FilterPluginDLL::Flag::AlwaysActive |
			AviUtl::FilterPluginDLL::Flag::DispFilter |
			AviUtl::FilterPluginDLL::Flag::ExInformation,
		.name = name,
		.track_n = sizeof(track_name) / sizeof(*track_name),
		.track_name = track_name,
		.track_default = track_def,
		.track_s = track_min,
		.track_e = track_max,
		.check_n = sizeof(check_name) / sizeof(*check_name),
		.check_name = check_name,
		.check_default = check_def,
		.func_init = func_init,
		.func_exit = func_exit,
		.func_WndProc = func_WndProc,
		.information = information,
	};

	return &filter;
}

//--------------------------------------------------------------------

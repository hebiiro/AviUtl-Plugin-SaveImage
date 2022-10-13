#pragma once

//--------------------------------------------------------------------

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

//--------------------------------------------------------------------

extern AviUtlInternal g_auin;
extern AviUtl::FilterPlugin* g_exedit;

//--------------------------------------------------------------------

BOOL onCommand(int commandIndex, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp);

//--------------------------------------------------------------------

#pragma once

//--------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)

struct ColorRGB { BYTE b, g, r; };
struct ColorRGBA { BYTE b, g, r, a; };

#pragma pack(pop)

//--------------------------------------------------------------------

class ImageSaver
{
private:

	AviUtl::EditHandle* m_editp = 0;
	AviUtl::FilterPlugin* m_fp = 0;
	int m_command = 0;

public:

	ImageSaver(AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp, int command);

	BOOL main();
};

//--------------------------------------------------------------------

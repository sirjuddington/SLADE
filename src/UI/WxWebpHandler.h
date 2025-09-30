#pragma once

#if !wxCHECK_VERSION(3, 3, 0) // WebP support is built in to wx in 3.3.0+

// Adapted from https://github.com/hoehermann/wxWEBPHandler
class WxWebpHandler : public wxImageHandler
{
public:
	inline WxWebpHandler()
	{
		m_name      = wxT("WebP file");
		m_extension = wxT("webp");
		// m_type = wxBITMAP_TYPE_WEBP; type omitted for now
		m_mime = wxT("image/webp");
	}

	virtual bool LoadFile(wxImage* image, wxInputStream& stream, bool verbose = true, int index = -1) wxOVERRIDE;
	virtual bool SaveFile(wxImage* image, wxOutputStream& stream, bool verbose = true) wxOVERRIDE;

protected:
	virtual bool DoCanRead(wxInputStream& stream) wxOVERRIDE;
	virtual int  DoGetImageCount(wxInputStream& stream) wxOVERRIDE;

private:
	wxDECLARE_DYNAMIC_CLASS(WxWebpHandler);
};

#endif

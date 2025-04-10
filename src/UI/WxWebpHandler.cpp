
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WxWebpHandler.cpp
// Description: wxImageHandler for WebP images.
//              Adapted from https://github.com/hoehermann/wxWEBPHandler
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "WxWebpHandler.h"
#include <functional>
#include <memory>
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>
#include <wx/mstream.h>

wxIMPLEMENT_DYNAMIC_CLASS(WxWebpHandler, wxImageHandler);

typedef std::unique_ptr<WebPDemuxer, std::function<void(WebPDemuxer*)>> WebPDemuxerPtr;

bool DecodeWebPDataIntoImage(wxImage* image, WebPData* webp_data, bool verbose)
{
	WebPBitstreamFeatures features;
	VP8StatusCode         status = WebPGetFeatures(webp_data->bytes, webp_data->size, &features);
	if (status != VP8_STATUS_OK)
	{
		if (verbose)
		{
			wxLogError("WebP: GetFeatures not OK.");
		}
		return false;
	}
	image->Create(features.width, features.height, false); // this allocates memory
	if (features.has_alpha)
	{
		// image has alpha channel. needs to be decoded, then re-ordered.
		uint8_t* rgba = WebPDecodeRGBA(webp_data->bytes, webp_data->size, &features.width, &features.height);
		if (nullptr == rgba)
		{
			if (verbose)
			{
				wxLogError("WebP: WebPDecodeRGBA failed.");
			}
			return false;
		}
		image->InitAlpha();
		unsigned char* rgb        = image->GetData();
		unsigned char* alpha      = image->GetAlpha();
		size_t         rgba_index = 0, rgb_index = 0, alpha_index = 0;
		for (int pixel_counter = 0; pixel_counter < image->GetWidth() * image->GetHeight(); pixel_counter++)
		{
			rgb[rgb_index++]     = rgba[rgba_index++]; // R
			rgb[rgb_index++]     = rgba[rgba_index++]; // G
			rgb[rgb_index++]     = rgba[rgba_index++]; // B
			alpha[alpha_index++] = rgba[rgba_index++]; // A
		}
		WebPFree(rgba);
	}
	else
	{
		// image has no alpha channel. decode into target buffer directly.
		int      buffer_size   = image->GetWidth() * image->GetHeight() * 3;
		int      stride        = image->GetWidth() * 3;
		uint8_t* output_buffer = WebPDecodeRGBInto(
			webp_data->bytes, webp_data->size, image->GetData(), buffer_size, stride);
		if (nullptr == output_buffer)
		{
			if (verbose)
			{
				wxLogError("WebP: WebPDecodeRGBInto failed.");
			}
			return false;
		}
	}
	image->SetMask(false); // all examples do this, so I do so as well
	return true;
}

bool DecodeWebPFrameIntoImage(wxImage* image, int index, WebPDemuxerPtr& demuxer, bool verbose)
{
	bool ok = false;
	// apparently, index can be -1 for "don't care", but libwebp does care
	if (index < 0)
	{
		index = 0;
	}
	WebPIterator iter;
	// wxImageHandler index starts from 0 (first frame)
	// WebPDemuxGetFrame to starts from 1 (0 means "last frame")
	if (WebPDemuxGetFrame(demuxer.get(), index + 1, &iter))
	{
		ok = DecodeWebPDataIntoImage(image, &iter.fragment, verbose);
		WebPDemuxReleaseIterator(&iter);
	}
	return ok;
}

WebPDemuxerPtr CreateDemuxer(wxInputStream& stream, bool verbose = false)
{
	wxMemoryOutputStream* mos = new wxMemoryOutputStream;
	stream.Read(*mos); // this reads the entire file into memory
	// TODO: Only read data as needed. WebPDemux can operate on partial data.
	//       Could save some bandwidth with e.g. DoGetImageCount
	wxStreamBuffer* mosb      = mos->GetOutputStreamBuffer();
	WebPData*       webp_data = new WebPData;
	webp_data->bytes          = reinterpret_cast<uint8_t*>(mosb->GetBufferStart());
	webp_data->size           = mosb->GetBufferSize();
	WebPDemuxerPtr demux(
		WebPDemux(webp_data),
		[mos, webp_data](WebPDemuxer* demux)
		{
			// delete the demuxer
			WebPDemuxDelete(demux);
			// delete the buffers after the WebPDemuxer is deleted
			delete webp_data;
			delete mos;
		});
	if (demux == nullptr)
	{
		// creating the demuxer failed, but the buffers still exist
		// and need to be cleaned up
		delete webp_data;
		delete mos;
		if (verbose)
		{
			wxLogError("WebP: WebPDemux failed.");
		}
	}
	return demux;
}

bool WxWebpHandler::LoadFile(wxImage* image, wxInputStream& stream, bool verbose, int index)
{
	image->Destroy(); // all examples do this, so I do so as well
	bool           ok    = false;
	WebPDemuxerPtr demux = CreateDemuxer(stream, verbose);
	if (nullptr != demux)
	{
		ok = DecodeWebPFrameIntoImage(image, index, demux, verbose);
	}
	return ok;
}

bool WxWebpHandler::SaveFile(wxImage* image, wxOutputStream& stream, bool)
{
	float quality_factor = 90;
	if (image->HasOption(wxIMAGE_OPTION_QUALITY))
	{
		quality_factor = image->GetOptionInt(wxIMAGE_OPTION_QUALITY);
	}
	size_t         output_size = 0;
	uint8_t*       output      = nullptr;
	unsigned char* rgb         = image->GetData();
	if (image->HasAlpha())
	{
		unsigned char*             alpha  = image->GetAlpha();
		int                        stride = image->GetWidth() * 4; // stride is the "width" of a "line" in bytes
		std::vector<unsigned char> rgba(stride * image->GetHeight());
		size_t                     rgba_index = 0, rgb_index = 0, alpha_index = 0;
		for (int pixel_counter = 0; pixel_counter < image->GetWidth() * image->GetHeight(); pixel_counter++)
		{
			rgba[rgba_index++] = rgb[rgb_index++];     // R
			rgba[rgba_index++] = rgb[rgb_index++];     // G
			rgba[rgba_index++] = rgb[rgb_index++];     // B
			rgba[rgba_index++] = alpha[alpha_index++]; // A
		}
		output_size = WebPEncodeRGBA(
			rgba.data(), image->GetWidth(), image->GetHeight(), stride, quality_factor, &output);
	}
	else
	{
		int stride  = image->GetWidth() * 3; // stride is the "width" of a "line" in bytes
		output_size = WebPEncodeRGB(rgb, image->GetWidth(), image->GetHeight(), stride, quality_factor, &output);
	}
	stream.WriteAll(output, output_size);
	WebPFree(output);
	return (output_size > 0 && stream.LastWrite() == output_size);
}

int WxWebpHandler::DoGetImageCount(wxInputStream& stream)
{
	int            frame_count = -1;
	WebPDemuxerPtr demux       = CreateDemuxer(stream);
	if (nullptr != demux)
	{
		frame_count = WebPDemuxGetI(demux.get(), WEBP_FF_FRAME_COUNT);
	}
	return frame_count;
}

bool WxWebpHandler::DoCanRead(wxInputStream& stream)
{
	// check header according to https://developers.google.com/speed/webp/docs/riff_container
	const std::string riff        = "RIFF";
	const std::string webp        = "WEBP";
	const int         buffer_size = 12;
	char              buffer[buffer_size];
	// it's ok to modify the stream position here
	stream.Read(buffer, buffer_size);
	if (stream.LastRead() != buffer_size)
	{
		return false;
	}
	return std::string(buffer, 4) == riff && std::string(&buffer[8], 4) == webp;
}

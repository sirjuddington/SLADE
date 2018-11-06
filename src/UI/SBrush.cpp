
/*******************************************************************
* SLADE - It's a Doom Editor
* Copyright (C) 2008-2017 Simon Judd
*
* Email:       sirjuddington@gmail.com
* Web:         http://slade.mancubus.net
* Filename:    SBrush.cpp
* Description: SBrush class. Handles pixel painting for GfxCanvas.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*******************************************************************/


/*******************************************************************
* INCLUDES
*******************************************************************/
#include "Main.h"
#include "SBrush.h"
#include "General/SAction.h"
#include "Graphics/SImage/SImage.h"
#include "Archive/ArchiveManager.h"


/*******************************************************************
* CONSTANTS
*******************************************************************/
SBrushManager* SBrushManager::instance = nullptr;

/*******************************************************************
* VARIABLES
*******************************************************************/

/*******************************************************************
* EXTERNAL VARIABLES
*******************************************************************/

/*******************************************************************
* SBRUSH CLASS FUNCTIONS
*******************************************************************/

/* SBrush::SBrush
 * SBrush class constructor
 *******************************************************************/

SBrush::SBrush(string name)
{

	image = nullptr;
	this->name = name;
	icon = name.AfterFirst('_');
	cx = 0;
	cy = 0;

	Archive* res = App::archiveManager().programResourceArchive();
	if (res == nullptr)
		return;
	ArchiveEntry* file = res->entryAtPath(S_FMT("icons/general/%s.png", icon));
	if (file == nullptr || file->getSize() == 0)
	{
		LOG_MESSAGE(2, "error, no file at icons/general/%s.png", icon);
		return;
	}
	image = new SImage();
	if (!image->open(file->getMCData(), 0, "png"))
	{
		LOG_MESSAGE(2, "couldn't load image data for icons/general/%s.png", icon);
		return;
	}
	image->convertAlphaMap(SImage::ALPHA);
	cx = image->getWidth() >> 1;
	cy = image->getHeight() >> 1;

	theBrushManager->add(this);
}

SBrush::~SBrush()
{
	if (image)
		delete image;
}

uint8_t SBrush::getPixel(int x, int y)
{
	x += cx;
	y += cy;
	if (image && x >= 0 && x < image->getWidth() && y >= 0 && y < image->getHeight())
		return image->getPixelIndex((unsigned)x, (unsigned)y);
	return 0;
}

SBrushManager::SBrushManager()
{
	brushes.clear();
}

SBrushManager::~SBrushManager()
{
	for (size_t i = brushes.size(); i > 0; --i)
	{
		delete brushes[i - 1];
		brushes[i - 1] = nullptr;
	}
	brushes.clear();
}

SBrush * SBrushManager::get(string name)
{
	for (size_t i = 0; i < brushes.size(); ++i)
		if (S_CMPNOCASE(name, brushes[i]->getName()))
			return brushes[i];

	return nullptr;
}

void SBrushManager::add(SBrush * brush)
{
	brushes.push_back(brush);
}

bool SBrushManager::initBrushes()
{
	new SBrush("pgfx_brush_sq_1");
	new SBrush("pgfx_brush_sq_3");
	new SBrush("pgfx_brush_sq_5");
	new SBrush("pgfx_brush_sq_7");
	new SBrush("pgfx_brush_sq_9");
	new SBrush("pgfx_brush_ci_5");
	new SBrush("pgfx_brush_ci_7");
	new SBrush("pgfx_brush_ci_9");
	new SBrush("pgfx_brush_di_3");
	new SBrush("pgfx_brush_di_5");
	new SBrush("pgfx_brush_di_7");
	new SBrush("pgfx_brush_di_9");
	new SBrush("pgfx_brush_pa_a");
	new SBrush("pgfx_brush_pa_b");
	new SBrush("pgfx_brush_pa_c");
	new SBrush("pgfx_brush_pa_d");
	new SBrush("pgfx_brush_pa_e");
	new SBrush("pgfx_brush_pa_f");
	new SBrush("pgfx_brush_pa_g");
	new SBrush("pgfx_brush_pa_h");
	new SBrush("pgfx_brush_pa_i");
	new SBrush("pgfx_brush_pa_j");
	new SBrush("pgfx_brush_pa_k");
	new SBrush("pgfx_brush_pa_l");
	new SBrush("pgfx_brush_pa_m");
	new SBrush("pgfx_brush_pa_n");
	new SBrush("pgfx_brush_pa_o");
	return true;
}

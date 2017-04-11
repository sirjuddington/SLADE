#pragma once

#include "Archive/Archive.h"

class MapEditContext;
class MapTextureManager;
class MapEditorWindow;
class MapObject;
class ObjectEditGroup;
class UndoManager;
class MapBackupManager;

namespace MapEditor
{
	/*struct Selection3D
	{
		int		index;
		uint8_t	type;

		bool operator<(const Selection3D& other) const {
			if (this->type == other.type)
				return this->index < other.index;
			else
				return this->type < other.type;
		}
	};*/

	MapEditContext&		editContext();
	MapTextureManager&	textureManager();
	MapEditorWindow*	window();
	wxWindow*			windowWx();
	MapBackupManager&	backupManager();

	void	init();
	void	forceRefresh(bool renderer = false);
	bool	chooseMap(Archive* archive = nullptr);
	void	setUndoManager(UndoManager* manager);

	void	openObjectProperties(MapObject* object);
	void	openMultiObjectProperties(vector<MapObject*>& objects);

	void	showShapeDrawPanel(bool show = true);
	void	showObjectEditPanel(bool show = true, ObjectEditGroup* group = nullptr);
}

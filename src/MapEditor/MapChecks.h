
#ifndef __MAP_CHECKS_H__
#define __MAP_CHECKS_H__

class SLADEMap;
class MapTextureManager;
class MapObject;
class MapEditor;

class MapCheck
{
protected:
	SLADEMap* map;

public:
	MapCheck(SLADEMap* map) { this->map = map; }
	virtual ~MapCheck() {}

	virtual void		doCheck() = 0;
	virtual unsigned	nProblems() = 0;
	virtual string		problemDesc(unsigned index) = 0;
	virtual bool		fixProblem(unsigned index, unsigned fix_type, MapEditor* editor) = 0;
	virtual MapObject*	getObject(unsigned index) = 0;
	virtual string		progressText() { return "Checking..."; }
	virtual string		fixText(unsigned fix_type, unsigned index) { return ""; }

	static MapCheck*	missingTextureCheck(SLADEMap* map);
	static MapCheck*	specialTagCheck(SLADEMap* map);
	static MapCheck*	intersectingLineCheck(SLADEMap* map);
	static MapCheck*	overlappingLineCheck(SLADEMap* map);
	static MapCheck*	overlappingThingCheck(SLADEMap* map);
	static MapCheck*	unknownTextureCheck(SLADEMap* map, MapTextureManager* texman);
	static MapCheck*	unknownFlatCheck(SLADEMap* map, MapTextureManager* texman);
	static MapCheck*	unknownThingTypeCheck(SLADEMap* map);
	static MapCheck*	stuckThingsCheck(SLADEMap* map);
	static MapCheck*	sectorReferenceCheck(SLADEMap* map);
	static MapCheck*	invalidLineCheck(SLADEMap* map);
	static MapCheck*	missingTaggedCheck(SLADEMap* map);
	static MapCheck*	unknownSectorCheck(SLADEMap* map);
	static MapCheck*	unknownSpecialCheck(SLADEMap* map);
	static MapCheck*	obsoleteThingCheck(SLADEMap* map);
};

#endif//__MAP_CHECKS_H__

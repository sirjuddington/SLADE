#pragma once

class SLADEMap;
class MapTextureManager;
class MapObject;
class MapEditContext;

class MapCheck
{
public:
	enum StandardCheck
	{
		MissingTexture,
		SpecialTag,
		IntersectingLine,
		OverlappingLine,
		OverlappingThing,
		UnknownTexture,
		UnknownFlat,
		UnknownThingType,
		StuckThing,
		SectorReference,
		InvalidLine,
		MissingTagged,
		UnknownSector,
		UnknownSpecial,
		ObsoleteThing,

		NumStandardChecks
	};

	MapCheck(SLADEMap* map) : map_{ map } {}
	virtual ~MapCheck() = default;

	virtual void       doCheck()                                                             = 0;
	virtual unsigned   nProblems()                                                           = 0;
	virtual string     problemDesc(unsigned index)                                           = 0;
	virtual bool       fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) = 0;
	virtual MapObject* getObject(unsigned index)                                             = 0;
	virtual string     progressText() { return "Checking..."; }
	virtual string     fixText(unsigned fix_type, unsigned index) { return ""; }

	static unique_ptr<MapCheck> standardCheck(StandardCheck type, SLADEMap* map, MapTextureManager* texman = nullptr);
	static unique_ptr<MapCheck> standardCheck(string_view type_id, SLADEMap* map, MapTextureManager* texman = nullptr);
	static string               standardCheckDesc(StandardCheck type);
	static string               standardCheckId(StandardCheck type);

protected:
	SLADEMap* map_;
};

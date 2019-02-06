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
	virtual wxString   problemDesc(unsigned index)                                           = 0;
	virtual bool       fixProblem(unsigned index, unsigned fix_type, MapEditContext* editor) = 0;
	virtual MapObject* getObject(unsigned index)                                             = 0;
	virtual wxString   progressText() { return "Checking..."; }
	virtual wxString   fixText(unsigned fix_type, unsigned index) { return ""; }

	typedef std::unique_ptr<MapCheck> UPtr;

	static UPtr     standardCheck(StandardCheck type, SLADEMap* map, MapTextureManager* texman = nullptr);
	static UPtr     standardCheck(const wxString& type_id, SLADEMap* map, MapTextureManager* texman = nullptr);
	static wxString standardCheckDesc(StandardCheck type);
	static wxString standardCheckId(StandardCheck type);

protected:
	SLADEMap* map_;
};

-- Changes all monsters in a map to Archviles
-- ----------------------------------------------------------------------------

-- Get map things
local things = App.mapEditor().map.things

-- Loop through all things
for i,thing in ipairs(things) do
    -- Change to type 64 (Archvile) if the thing is in the 'Monsters' group
	-- (or any sub-group)
    local type = Game.thingType(thing.type)
	if string.find(string.lower(type.group), 'monsters') == 1 then
		thing:setIntProperty('type', 64);
    end
end

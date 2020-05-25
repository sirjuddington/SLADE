<subhead>Example</subhead>
<header>Archvile Madness</header>

This example script changes all monsters in a map to Archviles.

```lua
-- Get map things
local things = App.MapEditor().map.things

-- Loop through all things
for _,thing in ipairs(things) do
   -- Change to type 64 (Archvile) if the thing is in the 'Monsters' group
   -- (or any sub-group)
   local type = Game.ThingType(thing.type)
   if string.find(string.lower(type.group), 'monsters') == 1 then
      thing:SetIntProperty('type', 64);
   end
end
```

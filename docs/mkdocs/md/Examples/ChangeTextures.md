<subhead>Example</subhead>
<header>Change Textures</header>

This example script changes all wall textures in a map to `AASHITTY` and all floor/ceiling textures to `SLIME09`.

```lua
-- Set textures to use
local wallTex = 'AASHITTY'
local flatTex = 'SLIME09'

-- Get map sidedefs
local sides = App.MapEditor().map.sidedefs

-- Loop through all sidedefs
for _,side in ipairs(sides) do
   -- Replace the middle texture if it is not blank (-)
   if side.textureMiddle ~= '-' then
      side:SetStringProperty('texturemiddle', wallTex)
   end

   -- Replace the upper texture if it is not blank (-)
   if side.textureTop ~= '-' then
      side:SetStringProperty('texturetop', wallTex)
   end

   -- Replace the lower texture if it is not blank (-)
   if side.textureBottom ~= '-' then
      side:SetStringProperty('texturebottom', wallTex)
   end
end

-- Get map sectors
local sectors = App.MapEditor().map.sectors

-- Loop through all sectors
for _,sector in ipairs(sectors) do
   -- Set ceiling texture
   sector:SetStringProperty('textureceiling', flatTex)

   -- Set floor texture
   sector:SetStringProperty('texturefloor', flatTex)
end
```

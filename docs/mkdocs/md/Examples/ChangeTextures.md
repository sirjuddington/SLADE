
This example script changes all wall textures in a map to `AASHITTY` and all floor/ceiling textures to `SLIME09`

```lua
-- Set textures to use
local walltex = 'AASHITTY'
local flattex = 'SLIME09'

-- Get map sidedefs
local sides = App.mapEditor().map.sidedefs

-- Loop through all sidedefs
for i,side in ipairs(sides) do
    -- Replace the middle texture if it is not blank (-)
    if side.textureMiddle ~= '-' then
		side:setStringProperty('texturemiddle', walltex)
    end

    -- Replace the upper texture if it is not blank (-)
    if side.textureTop ~= '-' then
		side:setStringProperty('texturetop', walltex)
    end

    -- Replace the lower texture if it is not blank (-)
    if side.textureBottom ~= '-' then
		side:setStringProperty('texturebottom', walltex)
    end
end

-- Get map sectors
local sectors = App.mapEditor().map.sectors

-- Loop through all sectors
for i,sector in ipairs(sectors) do
    -- Set ceiling texture
    sector:setStringProperty('textureceiling', flattex)

    -- Set floor texture
    sector:setStringProperty('texturefloor', flattex)
end
```

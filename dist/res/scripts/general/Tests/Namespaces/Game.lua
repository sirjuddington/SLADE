-- Tests for everything in the Game namespace

local title = 'Game Namespace Test'

local ttype = Game.ThingType(1)
if ttype == nil then
   App.LogMessage('No ThingType 1 found (most likely no game configuration is currently loaded)')
else
   App.LogMessage('ThingType 1 name: ' .. ttype.name)
end

UI.MessageBox(title, 'Game namespace test completed successfully')

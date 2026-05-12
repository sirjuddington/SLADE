-- Tests for everything in the App namespace

App.LogMessage("App.LogMessage test")
App.LogWarning("App.LogWarning test")
App.LogError("App.LogError test")

local current_archive = App.CurrentArchive()
if current_archive == nil then
	App.LogMessage('No current archive')
else
	App.LogMessage('Current archive: ' .. current_archive.filename)
end

local current_entry = App.CurrentEntry()
if current_entry == nil then
	App.LogMessage('No current entry')
else
	App.LogMessage('Current entry: ' .. current_entry.name)
end

local entry_selection = App.CurrentEntrySelection()
App.LogMessage(#entry_selection .. ' selected entries:')
for _,entry in ipairs(entry_selection) do
	App.LogMessage('  ' .. entry.name)
end

local current_palette = App.CurrentPalette()
if current_palette == nil then
	App.LogMessage('No current palette (no base resource selected?)')
else
	App.LogMessage('Current palette has ' .. current_palette.colourCount .. ' colours')
end

if current_archive ~= nil then
	App.LogMessage('Showing the current archive...')
	App.ShowArchive(current_archive)
end

if current_entry ~= nil then
	App.LogMessage('Opening the current entry in a tab...')
	App.ShowEntry(current_entry)
elseif #entry_selection > 0 then
	App.LogMessage('Opening the first selected entry in a tab...')
	App.ShowEntry(entry_selection[1])
else
	App.LogMessage('No available entry to open')
end

local map_editor = App.MapEditor()
if map_editor == nil then
	App.LogMessage('Map Editor not currently open')
else
	App.LogMessage('Map Editor open, map: ' .. map_editor.map.name)
end

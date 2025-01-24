-- Tests for the Archive type

local title = 'Archive Test'
local failed_count = 0

local function ListEntries(archive)
   for i = 1, math.min(50, #archive.entries) do
      App.LogMessage('  ' .. i .. ': ' .. archive.entries[i]:FormattedName())
   end
end


-- Open resource archive for tests
local archive = Archives.ProgramResource()
if archive == nil then
   App.LogMessage('No program resource archive loaded (how?)')
   UI.MessageBox(title, 'Archive test failed: No program resource archive loaded')
   return
end

-- Properties
App.LogMessage('Archive properties test:')
App.LogMessage('  filename: ' .. archive.filename)
App.LogMessage('  entries (count): ' .. #archive.entries)
App.LogMessage('  rootDir: ' .. archive.rootDir.path)
App.LogMessage('  format: ' .. archive.format.name)

-- DirAtPath
local dir = archive:DirAtPath('scripts/general')
if dir == nil then
   App.LogMessage('No dir scripts/general found (should exist)')
   failed_count = failed_count + 1
else
   App.LogMessage('Got dir ' .. dir.path .. ' (has ' .. #dir.entries .. ' entries and ' .. #dir.subDirectories .. ' subdirs)')
end

-- EntryAtPath
local entry = archive:EntryAtPath('images/STFDEAD0.png')
if entry == nil then
   App.LogMessage('No entry images/STFDEAD0.png found (should exist)')
   failed_count = failed_count + 1
else
   App.LogMessage('Got entry ' .. entry:FormattedName() .. ', size: ' .. entry:FormattedSize())
end

-- FilenameNoPath
App.LogMessage('FilenameNoPath: ' .. archive:FilenameNoPath())

-- FindFirst
local search_opt = ArchiveSearchOptions()
search_opt.matchType = Archives.EntryType('png')
search_opt.searchSubdirs = true
entry = archive:FindFirst(search_opt)
if entry == nil then
   App.LogMessage('No png entry found, should exist')
   failed_count = failed_count + 1
else
   App.LogMessage('FindFirst found entry ' .. entry:FormattedName())
end

-- FindLast
entry = archive:FindLast(search_opt)
if entry == nil then
   App.LogMessage('No png entry found, should exist')
   failed_count = failed_count + 1
else
   App.LogMessage('FindLast found entry ' .. entry:FormattedName())
end

-- FindAll
local found_entries = archive:FindAll(search_opt)
if #found_entries == 0 then
   App.LogMessage('No png entries found, should exist')
   failed_count = failed_count + 1
else
   App.LogMessage('FindAll found ' .. #found_entries .. ' entries, first 10:')
   for i = 1, math.min(10, #found_entries) do
      App.LogMessage('  ' .. found_entries[i]:FormattedName())
   end
end


-- Create new zip archive for modification tests
archive = Archives.Create('zip')
App.LogMessage('Created zip archive, all entries:')
ListEntries(archive)

-- CreateDir
local dir = archive:CreateDir('textures')
App.LogMessage('Created dir ' .. dir.name .. ', all entries:')
ListEntries(archive)

-- CreateEntry
entry = archive:CreateEntry('test1.txt', 0)
App.LogMessage('Created entry ' .. entry.name .. ', all entries:')
ListEntries(archive)
entry = archive:CreateEntry('test2.txt', -1)
App.LogMessage('Created entry ' .. entry.name .. ' at end, all entries:')
ListEntries(archive)
entry = archive:CreateEntry('textures/tex01.png', 0)
App.LogMessage('Created entry ' .. entry.name .. ' in textures/, all entries:')
ListEntries(archive)

-- CreateEntryInNamespace
entry = archive:CreateEntryInNamespace('tex02.png', 'textures')
App.LogMessage('Created entry ' .. entry.name .. ' in textures namespace, all entries:')
ListEntries(archive)

-- RenameEntry
archive:RenameEntry(entry, 'tex11.png')
if entry.name ~= 'tex11.png' then
   App.LogMessage('Failed to rename tex02.png to tex11.png, entry name is ' .. entry.name .. ', all entries:')
   failed_count = failed_count + 1
else
   App.LogMessage('Renamed tex02.png to ' .. entry.name .. ', all entries:')
end
ListEntries(archive)

-- RemoveEntry
archive:RemoveEntry(entry)
App.LogMessage('Removed entry ' .. entry:FormattedName() .. ', all entries:')
ListEntries(archive)



if failed_count == 0 then
   UI.MessageBox(title, 'Archive test completed successfully')
else
   UI.MessageBox(title,
      'Archive test completed with ' .. failed_count .. ' failed test(s), see console log for details',
      UI.MB_ICON_ERROR)
end

-- Tests for everything in the Archives namespace

local string title = 'Archives Namespace Test'

local function TestFail()
   UI.MessageBox(title, 'Archives namespace test failed - see console log for details', UI.MB_ICON_ERROR)
end

local function ListOpenArchives()
   App.LogMessage('All open archives:')
   for i,archive in ipairs(Archives.All()) do
      App.LogMessage('  ' .. i .. ': ' .. archive.filename)
   end
end

local function ListEntries(archive)
   for i = 1, math.min(5, #archive.entries) do
      App.LogMessage('  ' .. i .. ': ' .. archive.entries[i].name)
   end
end

local function ListBookmarks()
   App.LogMessage('All bookmarks:')
   for i,bookmark in ipairs(Archives.Bookmarks()) do
      App.LogMessage('  ' .. i .. ': ' .. bookmark.name)
   end
end

-- This test requires all archives to be closed initially, prompt
if UI.PromptYesNo(title, 'To run this test all archives must be closed, continue running script and close all open archives?') then
   Archives.CloseAll()
else
   return
end

-- Create archive (invalid format)
local archive, error = Archives.Create('dat')
if archive == nil then
   App.LogMessage('Create with invalid archive format: Failed as expected, error: ' .. error)
else
   App.LogError('Create with invalid archive format: Did not fail, something is wrong')
   TestFail()
   return
end

-- Create archive (valid format)
archive, error = Archives.Create('wad')
if archive == nil then
   App.LogError('Create with valid archive format: Failed')
   TestFail()
   return
else
   App.LogMessage('Create with valid archive format: Passed, archive created successfully')
end

-- BaseResource
archive = Archives.BaseResource()
if archive == nil then
   App.LogMessage('No base resource archive loaded')
else
   App.LogMessage('Base resource archive: ' .. archive.filename)
end

-- BaseResourcePaths
local base_resource_paths = Archives.BaseResourcePaths()
for i,path in ipairs(base_resource_paths) do
   App.LogMessage('Base resource path ' .. i .. ': ' .. path)
end

-- OpenBaseResource (TODO - need a way to restore previous selection)

-- OpenFile with invalid path
archive, error = Archives.OpenFile('notafile_.naf')
if archive == nil then
   App.LogMessage('Open with invalid path: Failed as expected, error: ' .. error)
else
   App.LogError('Open with invalid path: Did not fail, something is wrong (or the file actually exists and is an archive somehow?)')
   TestFail()
   return
end

-- OpenFile with valid path (hopefully)
if #base_resource_paths > 0 then
   archive, error = Archives.OpenFile(base_resource_paths[1])
   if archive == nil then
      App.LogMessage('Open with valid path: Failed to open, error: ' .. error)
   else
      App.LogMessage('Open with valid path: Passed, opened first base resource successfully')
   end
else
   App.LogMessage('Open with valid path: No base resource archives configured, skipping test')
end

ListOpenArchives()

-- Add first 2 entries in the last open archive to bookmarks
if archive == nil then
   App.LogMessage('No open archives to add bookmarks to')
else
   App.LogMessage('Adding first 2 entries of last open archive to bookmarks...')
   for i = 1, math.min(2, #archive.entries) do
      Archives.AddBookmark(archive.entries[i])
   end
end

ListBookmarks()

-- Remove first bookmark
if #Archives.Bookmarks() > 0 then
   App.LogMessage('Removing first bookmark...')
   Archives.RemoveBookmark(Archives.Bookmarks()[1])
end

ListBookmarks()

-- Close the first open archive (should be the one created earlier)
App.LogMessage('Closing first open archive...')
Archives.Close(0)

ListOpenArchives()

-- Close all open archives
App.LogMessage('Closing all open archives...')
Archives.CloseAll()

ListOpenArchives()

-- ProgramResource
archive = Archives.ProgramResource()
if archive == nil then
   App.LogMessage('No program resource archive loaded (how?)')
   TestFail()
   return
else
   App.LogMessage('Program resource archive top 5 entries:')
   ListEntries(archive)
end

-- List recent files
App.LogMessage('Recent files:')
for i,path in ipairs(Archives.RecentFiles()) do
   App.LogMessage('  ' .. i .. ': ' .. path)
end

-- EntryType
local entry_type = Archives.EntryType('wad')
App.LogMessage('Entry type wad, name: ' .. entry_type.name .. ', category: ' .. entry_type.category)

UI.MessageBox(title, 'Archives namespace test completed successfully')

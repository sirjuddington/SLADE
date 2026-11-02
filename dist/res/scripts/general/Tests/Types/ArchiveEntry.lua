
local title = 'ArchiveEntry Test'
local failed_count = 0

local res_archive = Archives.ProgramResource()
local res_entry = res_archive:EntryAtPath('images/STFDEAD0.png')

-- Properties
App.LogMessage('Properties of entry images/STFDEAD0.png in slade.pk3:')
App.LogMessage('  name: ' .. res_entry.name)
App.LogMessage('  path: ' .. res_entry.path)
App.LogMessage('  type: ' .. res_entry.type.name .. ' (' .. res_entry.type.id .. ')')
App.LogMessage('  size: ' .. res_entry.size)
App.LogMessage('  data (crc): ' .. res_entry.data.crc)
App.LogMessage('  index: ' .. res_entry.index)
App.LogMessage('  crc32: ' .. res_entry.crc32)
App.LogMessage('  parentArchive (filename): ' .. res_entry.parentArchive.filename)
App.LogMessage('  parentDir (path): ' .. res_entry.parentDir.path)

-- FormattedName
App.LogMessage('FormattedName (default): ' .. res_entry:FormattedName())
App.LogMessage('FormattedName (no path): ' .. res_entry:FormattedName(false))
App.LogMessage('FormattedName (no ext): ' .. res_entry:FormattedName(true, false))
App.LogMessage('FormattedName (uppercase): ' .. res_entry:FormattedName(true, true, true))

-- FormattedSize
App.LogMessage('FormattedSize: ' .. res_entry:FormattedSize())

-- TODO: ExportFile (need some way to get temp folder)

-- Create temp archive/entry to test entry modification
local archive = Archives.Create('zip')
local entry = archive:CreateEntry('test_entry', -1)

-- Rename
App.LogMessage('Entry name: ' .. entry.name)
entry:Rename('test_entry2')
if entry.name ~= 'test_entry2' then
   App.LogMessage('Rename test failed, new name ' .. entry.name .. ', should be test_entry2')
   failed_count = failed_count + 1
else
   App.LogMessage('Rename test passed, new name: ' .. entry.name)
end

-- ImportData (DataBlock)
App.LogMessage('Entry current size: ' .. entry.size .. ', crc: ' .. entry.crc32)
local ok, error = entry:ImportData(res_entry.data)
if not ok then
   App.LogMessage('ImportData (DataBlock) test failed: ' .. error)
   failed_count = failed_count + 1
elseif entry.size == 1624 and entry.crc32 == 2412926683 then
   App.LogMessage('ImportData (DataBlock) test passed, new entry size: ' .. entry.size .. ', crc: ' .. entry.crc32)
else
   App.LogMessage('ImportData (DataBlock) test failed, unexpected size/crc. New entry size: ' .. entry.size .. ', crc: ' .. entry.crc32)
   failed_count = failed_count + 1
end

-- ImportData (string)
App.LogMessage('Entry current size: ' .. entry.size .. ', crc: ' .. entry.crc32)
local ok, error = entry:ImportData('testing 123')
if not ok then
   App.LogMessage('ImportData (string) test failed: ' .. error)
   failed_count = failed_count + 1
elseif entry.size == 11 and entry.crc32 == 595584620 then
   App.LogMessage('ImportData (string) test passed, new entry size: ' .. entry.size .. ', crc: ' .. entry.crc32)
else
   App.LogMessage('ImportData (string) test failed, unexpected size/crc. New entry size: ' .. entry.size .. ', crc: ' .. entry.crc32)
   failed_count = failed_count + 1
end

-- ImportEntry
App.LogMessage('Entry current size: ' .. entry.size .. ', crc: ' .. entry.crc32)
local ok, error = entry:ImportEntry(res_entry)
if not ok then
   App.LogMessage('ImportEntry test failed: ' .. error)
   failed_count = failed_count + 1
elseif entry.size == 1624 and entry.crc32 == 2412926683 then
   App.LogMessage('ImportEntry test passed, new entry size: ' .. entry.size .. ', crc: ' .. entry.crc32)
else
   App.LogMessage('ImportEntry test failed, unexpected size/crc. New entry size: ' .. entry.size .. ', crc: ' .. entry.crc32)
   failed_count = failed_count + 1
end

-- TODO: ImportFile
-- Import the file that was exported to previously (once that is added)


Archives.Close(archive)


if failed_count == 0 then
   UI.MessageBox(title, 'ArchiveEntry test completed successfully')
else
   UI.MessageBox(title,
      'ArchiveEntry test completed with ' .. failed_count .. ' failed test(s), see console log for details',
      UI.MB_ICON_ERROR)
end

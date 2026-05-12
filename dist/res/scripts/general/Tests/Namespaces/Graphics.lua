-- Tests for everything in the Graphics namespace

local title = 'Graphics Namespace Test'
local failed_count = 0

-- AllImageFormats
local all_formats = Graphics.AllImageFormats()
App.LogMessage('All image formats:')
for i,format in ipairs(all_formats) do
   App.LogMessage('  ' .. i .. ': ' .. format.name .. ' (' .. format.id .. ')')
end

-- Invalid format
local format = Graphics.ImageFormat('abcdefghij')
if format == nil or format.id == 'unknown' then
   App.LogMessage('Invalid image format test: Passed, got invalid format as expected')
else
   App.LogMessage('Invalid image format test: Failed, got valid format ' .. format.id)
   failed_count = failed_count + 1
end

-- Valid format
format = Graphics.ImageFormat('png')
if format == nil or format.id ~= 'png' then
   App.LogMessage('Valid image format test: Failed, got invalid or wrong format')
   failed_count = failed_count + 1
else
   App.LogMessage('Valid image format test: Passed, got format ' .. format.id)
end

-- DetectImageFormat (using logo.png in slade.pk3)
local archive = Archives.ProgramResource()
local entry = archive:EntryAtPath('logo.png')
format = Graphics.DetectImageFormat(entry.data)
if format ~= nil and format.id == 'png' then
   App.LogMessage('Detect image format test: Passed, detected format ' .. format.id)
else
   App.LogMessage('Detect image format test: Failed, detected format ' .. (format and format.id or 'nil'))
   failed_count = failed_count + 1
end

-- GetImageInfo
local info = Graphics.GetImageInfo(entry.data)
if info == nil then
   App.LogMessage('Get image info test: Failed, got nil')
   failed_count = failed_count + 1
else
   App.LogMessage('Get image info test: Passed, got info:')
   for key,value in pairs(info) do
      App.LogMessage('  ' .. key .. ': ' .. tostring(value))
   end
end


if failed_count == 0 then
   UI.MessageBox(title, 'Graphics namespace test completed successfully')
else
   UI.MessageBox(title,
      'Graphics namespace test completed with ' .. failed_count .. ' failed test(s), see console log for details',
      UI.MB_ICON_ERROR)
end


-- Browse for archive file to open
local path = slade.browseFile('Open Archive', archives.fileExtensionsString(), '')
if path == '' then
    slade.logMessage('No archive selected')
else
    -- Open it
    local archive = archives.openFile(path)

    -- Check it opened ok
    if archive == nil then
        slade.logMessage('Archive not opened: ' .. slade.globalError())
    else
        slade.logMessage('Archive opened successfully')

        -- List all entries
        for i,entry in ipairs(archive.entries) do
            slade.logMessage(entry:formattedName() .. ' (' .. entry:formattedSize() .. ', ' .. entry.type.name .. ')')
        end

        -- Prompt to close
        if (slade.promptYesNo('Close Archive', 'Do you want to close the archive now?')) then
            archives.close(archive)
        end
    end
end

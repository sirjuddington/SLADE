
-- Browse for archive file to open
local path = slade.browseFile('Open Archive', slade.archiveManager():getArchiveExtensionsString(), '')
if path == '' then
    slade.logMessage('No archive selected')
else
    -- Open it
    local archive = slade.archiveManager():openFile(path)

    -- Check it opened ok
    if archive == nil then
        slade.logMessage('Archive not opened: ' .. slade.globalError())
    else
        slade.logMessage('Archive opened successfully')

        -- List all entries
        for i,entry in ipairs(archive:allEntries()) do
            slade.logMessage(entry:getPath(true) .. ' (' .. entry:getSizeString() .. ', ' .. entry:getTypeString() .. ')')
        end

        -- Prompt to close
        if (slade.promptYesNo('Close Archive', 'Do you want to close the archive now?')) then
            slade.archiveManager():closeArchive(archive)
        end
    end
end

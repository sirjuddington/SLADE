
-- Browse for archive file to open
local path = App.browseFile('Open Archive', Archives.fileExtensionsString(), '')
if path == '' then
    App.logMessage('No archive selected')
else
    -- Open it
    local archive = Archives.openFile(path)

    -- Check it opened ok
    if archive == nil then
        App.logMessage('Archive not opened: ' .. App.globalError())
    else
        App.logMessage('Archive opened successfully')

        -- List all entries
        for i,entry in ipairs(archive.entries) do
            App.logMessage(entry:formattedName() .. ' (' .. entry:formattedSize() .. ', ' .. entry.type.name .. ')')
        end

        -- Prompt to close
        if (App.promptYesNo('Close Archive', 'Do you want to close the archive now?')) then
            Archives.close(archive)
        end
    end
end

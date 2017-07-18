
This example script does the following:

1. Prompts the user to select a SLADE-supported archive file
1. Opens the selected archive
1. Lists details for all entries in the archive
1. Prompts the user to close the archive and closes it if they click Yes

```lua
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
        for i,entry in ipairs(archive:allEntries()) do
            slade.logMessage(entry:getPath(true) .. ' (' .. entry:getSizeString() .. ', ' .. entry:getTypeString() .. ')')
        end

        -- Prompt to close
        if (slade.promptYesNo('Close Archive', 'Do you want to close the archive now?')) then
            archives.close(archive)
        end
    end
end
```

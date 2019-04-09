<article-head>Example: Open Archive</article-head>

This example script does the following:

1. Prompts the user to select a SLADE-supported archive file
1. Opens the selected archive
1. Lists details for all entries in the archive
1. Prompts the user to close the archive and closes it if they click Yes

```lua
-- Browse for archive file to open
local path = UI.BrowseFile('Open Archive', Archives.FileExtensionsString(), '')
if path == '' then
   App.LogMessage('No archive selected')
else
   -- Open it
   local archive, error = Archives.OpenFile(path)

   -- Check it opened ok
   if archive == nil then
      App.LogMessage('Archive not opened: ' .. error)
   else
      App.LogMessage('Archive opened successfully')

      -- List all entries
      for _,entry in ipairs(archive.entries) do
         App.LogMessage(entry:FormattedName()..' ('..entry:FormattedSize()..', '..entry.type.name..')')
      end

      -- Prompt to close
      if (UI.PromptYesNo('Close Archive', 'Do you want to close the archive now?')) then
         Archives.Close(archive)
         archive = nil
      end
   end
end
```

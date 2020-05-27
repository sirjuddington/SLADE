-- Browse for archive file to open
local path = UI.PromptOpenFile("Open Archive", Archives.FileExtensionsString(), "")

if path == "" then
   App.LogMessage("No archive selected")
else
   -- Open it
   local archive, error = Archives.OpenFile(path)

   -- Check it opened ok
   if archive == nil then
      App.LogMessage("Archive not opened: " .. error)
   else
      App.LogMessage("Archive opened successfully")

      -- List all entries
      for _, entry in ipairs(archive.entries) do
         App.LogMessage(entry:FormattedName() .. " (" .. entry:FormattedSize() .. ", " .. entry.type.name .. ")")
      end

      -- Prompt to close
      if UI.PromptYesNo("Close Archive", "Do you want to close the archive now?") then
         Archives.Close(archive)
         archive = nil
      end
   end
end

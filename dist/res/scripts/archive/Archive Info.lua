
function ValidFor(archive)
    return true
end

local info = {}
local output = "Detailed Info:\n\n"

function CountEntry(entry)
   -- Increment total entry count
   info.entryCount = info.entryCount + 1

   -- Increment count and size for category
   local tbl = info.entryCategories[entry.type.category]
   if tbl == nil then
      info.entryCategories[entry.type.category] = { count=1, size=entry.size }
   else
      tbl.count = tbl.count + 1
      tbl.size = tbl.size + entry.size
   end
end

function AddOutputLine(line)
   output = output .. line .. "\n"
end

function YesNo(boolean)
   if boolean == true then
      return "Yes"
   else
      return "No"
   end
end

function Execute(archive)
   -- Archive format info
   AddOutputLine("Format: " .. archive.format.name)
   AddOutputLine("Supports directories: " .. YesNo(archive.format.supportsDirs))
   AddOutputLine("Entry names have extensions: " .. YesNo(archive.format.hasExtensions))
   if archive.format.maxNameLength > 0 then
      AddOutputLine("Max entry name length: " .. archive.format.maxNameLength)
   end
   AddOutputLine("")

   -- Process entries
   info.entryCount = 0
   info.entryCategories = {}
   for _,entry in ipairs(archive.entries) do
      CountEntry(entry)
   end

   -- Write entry info
   AddOutputLine("Entry count: " .. info.entryCount)
   AddOutputLine("")
   AddOutputLine("Entry count by category:")
   for category,catInfo in pairs(info.entryCategories) do
      AddOutputLine(string.format("%d %s (%d bytes)", catInfo.count, category, catInfo.size))
   end

   -- Display info output in message box
   UI.MessageBoxExt("Archive Info", "Information about " .. archive.filename .. ":", output)
end

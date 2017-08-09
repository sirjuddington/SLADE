
function validFor(archive)
    return true
end

local info = {}
local output = 'Detailed Info:\n\n'

function countEntry(entry)
	-- Increment total entry count
    info.entry_count = info.entry_count + 1
	
	-- Increment count and size for category
	local tbl = info.entry_categories[entry.type.category]
	if tbl == nil then
		info.entry_categories[entry.type.category] = { count=1, size=entry.size }
	else
		tbl.count = tbl.count + 1
		tbl.size = tbl.size + entry.size
	end
end

function addOutputLine(line)
	output = output .. line .. '\n'
end

function yesNo(boolean)
	if boolean == true then
		return 'Yes'
	else
		return 'No'
	end
end

function execute(archive)
	-- Archive format info
	addOutputLine('Format: ' .. archive.format.name)
	addOutputLine('Supports directories: ' .. yesNo(archive.format.supportsDirs))
	addOutputLine('Entry names have extensions: ' .. yesNo(archive.format.hasExtensions))
	if archive.format.maxNameLength > 0 then
		addOutputLine('Max entry name length: ' .. archive.format.maxNameLength)
	end
	addOutputLine('')

	-- Process entries
	info.entry_count = 0
	info.entry_categories = {}
	for i,entry in ipairs(archive.entries) do
        countEntry(entry)
    end

	-- Write entry info
	addOutputLine('Entry count: ' .. info.entry_count)
	addOutputLine('')
	addOutputLine('Entry count by category:')
	for category,c_info in pairs(info.entry_categories) do
		addOutputLine(string.format('%d %s (%d bytes)', c_info.count, category, c_info.size))
	end

	-- Display info output in message box
    App.messageBoxExt('Archive Info', 'Information about ' .. archive.filename .. ':', output)
end

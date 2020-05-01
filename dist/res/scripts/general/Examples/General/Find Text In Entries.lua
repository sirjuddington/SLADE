-- Function to find the line number of [position] in [text]
function lineNum(text, position)
    local line = 1
    for i = 1,position do
        if text:sub(i, i) == '\n' then
            line = line + 1
        end
    end
    return line
end

-- Function to find [term] in text [entry]
function findTerm(entry, term)
	-- Get entry text and search term as uppercase to make the search case-insensitive
	local text = string.upper(entry.data)
	local term_upper = string.upper(term)

	-- Search text
    local word_end = 1
	while true do
		i, word_end = text:find(term_upper, word_end)
        if i == nil then break end
        App.logMessage('Text "' .. term .. '" found in ' .. entry:formattedName() .. ' on line ' .. lineNum(text, i))
    end
end

-- Prompt for search term
local search = App.promptString('Find Text In Entries', 'Enter text to find in all entries', '')

-- Check the entered search term is long enough
if string.len(search) < 2 then
	App.logMessage('Search text too short, must be atleast 2 characters')
	return
end

-- Go through all entries in the currently selected archive
for i,entry in ipairs(App.currentArchive().entries) do
    -- Do search if the entry is opened in the text editor
    if entry.type.editor == 'text' then
        findTerm(entry, search)
    end
end

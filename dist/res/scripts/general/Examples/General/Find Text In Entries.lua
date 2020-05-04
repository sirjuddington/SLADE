-- Function to find the line number of [position] in [text]
function LineNum(text, position)
    local line = 1
    for i = 1,position do
        if text:sub(i, i) == '\n' then
            line = line + 1
        end
    end
    return line
end

-- Function to find [term] in text [entry]
function FindTerm(entry, term)
	-- Get entry text and search term as uppercase to make the search case-insensitive
	local text = string.upper(entry.data)
	local termUpper = string.upper(term)

	-- Search text
    local wordEnd = 1
	while true do
		i, wordEnd = text:find(termUpper, wordEnd)
        if i == nil then break end
        App.LogMessage('Text "' .. term .. '" found in ' .. entry:FormattedName() .. ' on line ' .. LineNum(text, i))
    end
end

-- Prompt for search term
local search = UI.PromptString('Find Text In Entries', 'Enter text to find in all entries', '')

-- Check the entered search term is long enough
if string.len(search) < 2 then
	App.LogMessage('Search text too short, must be atleast 2 characters')
	return
end

-- Go through all entries in the currently selected archive
for _,entry in ipairs(App.CurrentArchive().entries) do
    -- Do search if the entry is opened in the text editor
    if entry.type.editor == 'text' then
        FindTerm(entry, search)
    end
end

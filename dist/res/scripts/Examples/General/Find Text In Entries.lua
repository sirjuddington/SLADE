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
    local text = entry.data
    local word_end = 1
    while true do
        i, word_end = text:find(term, word_end)
        if i == nil then break end
        slade.logMessage('Text "' .. term .. '" found in ' .. entry:formattedName() .. ' on line ' .. lineNum(text, i))
    end
end

-- Prompt for search term
local search = slade.promptString('Find Text In Entries', 'Enter text to find in all entries', '')

-- Go through all entries in the currently selected archive
for i,entry in ipairs(slade.currentArchive().entries) do
    -- Do search if the entry is opened in the text editor
    if entry.type.editor == 'text' then
        findTerm(entry, search)
    end
end

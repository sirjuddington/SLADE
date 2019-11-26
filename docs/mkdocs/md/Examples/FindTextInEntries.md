<article-head>Example: Find Text in Entries</article-head>

This example script will prompt for a search term, then search all text entries in the currently selected archive for the term. Matches are logged to the console.

```lua
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
    local text = entry.data:AsString()
    local wordEnd = 1
    while true do
        i, wordEnd = text:find(term, wordEnd)
        if i == nil then break end
        App.LogMessage(
            'Text "'..term..'" found in '..entry:FormattedName()..' on line '..LineNum(text, i))
    end
end

-- Prompt for search term
local search = UI.PromptString('Find Text In Entries', 'Enter text to find in all entries', '')

-- Go through all entries in the currently selected archive
for _,entry in ipairs(App.CurrentArchive().entries) do
    -- Do search if the entry is opened in the text editor
    if entry.type.editor == 'text' then
        FindTerm(entry, search)
    end
end
```

-- Tests for everything in the UI namespace

local title = 'UI Namespace Test'
local extensions = 'All Files (*.*)|*.*'
local PauseMsgBox = function(msg) UI.MessageBox(title, msg or 'Pause for splash window test') end


-- MessageBoxes
UI.MessageBox(title, 'Messagebox with default icon (info)')
UI.MessageBox(title, 'Messagebox with info icon', UI.MB_ICON_INFO)
UI.MessageBox(title, 'Messagebox with question icon', UI.MB_ICON_QUESTION)
UI.MessageBox(title, 'Messagebox with warning icon', UI.MB_ICON_WARNING)
UI.MessageBox(title, 'Messagebox with error icon', UI.MB_ICON_ERROR)
UI.MessageBoxExt(title, 'This is an extended message box', 'This is the detail box for long text')


-- Prompts
local response = UI.PromptString(title, 'Enter a string', '(default value)')
App.LogMessage('PromptString response: ' .. response)

response = UI.PromptNumber(title, 'Enter a number between 1 and 100', 1, 1, 100)
App.LogMessage('PromptNumber response: ' .. response)

response = UI.PromptYesNo(title, 'Yes or No?')
App.LogMessage('PromptYesNo response: ' .. tostring(response))

response = UI.PromptOpenFile('Select a File', extensions, '')
App.LogMessage('PromptOpenFile response: ' .. response)

response = UI.PromptOpenFiles('Select Files', extensions)
App.LogMessage('PromptOpenFiles response:')
for _,filename in ipairs(response) do
   App.LogMessage('  ' .. filename)
end

response = UI.PromptSaveFile('Save File (no default name)', extensions)
App.LogMessage('PromptSaveFile response (no default name): ' .. response)

response = UI.PromptSaveFile('Save File (withdefault name)', extensions, 'doom2.wad')
App.LogMessage('PromptSaveFile response (with default name): ' .. response)

response, extension = UI.PromptSaveFiles('Save Files', extensions .. '|Text Files (*.txt)|*.txt')
App.LogMessage('PromptSaveFiles response: ' .. response .. ' (extension: ' .. extension .. ')')


-- Splash Window
UI.ShowSplash("Splash with no progress")
PauseMsgBox()
UI.ShowSplash("Splash with progress", true)
PauseMsgBox('Current progress: ' .. UI.SplashProgress())
UI.SetSplashMessage('New splash message')
PauseMsgBox()
UI.SetSplashProgress(0.5)
UI.SetSplashProgressMessage('Current progress: ' .. UI.SplashProgress())
PauseMsgBox()
UI.SetSplashProgress(-1)
UI.SetSplashProgressMessage('Indeterminate progress')
PauseMsgBox()
UI.HideSplash()
App.LogMessage('Splash window hidden')


UI.MessageBox(title, 'UI namespace test completed successfully')

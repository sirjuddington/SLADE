
local function PropertiesTest(dir)
   local function ParentDir(dir)
      if dir.parent == nil then
         return 'nil'
      else
         return dir.parent.path
      end
   end
   
   App.LogMessage('  name: ' .. dir.name)
   App.LogMessage('  archive (filename): ' .. dir.archive.filename)
   App.LogMessage('  entries (count): ' .. #dir.entries)
   App.LogMessage('  parent: ' .. ParentDir(dir))
   App.LogMessage('  path: ' .. dir.path)
   App.LogMessage('  subDirectories (count): ' .. #dir.subDirectories)
end


local archive = Archives.ProgramResource()
if archive == nil then
   App.LogMessage('No program resource archive loaded (how?)')
   UI.MessageBox(title, 'ArchiveDir test failed: No program resource archive loaded')
   return
end

-- Properties
App.LogMessage('ArchiveDir properties test (root dir):')
PropertiesTest(archive.rootDir)
App.LogMessage('ArchiveDir properties test (non-root dir):')
PropertiesTest(archive:DirAtPath('icons/general'))

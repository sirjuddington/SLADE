The **ArchiveDir** type represents a 'directory' of entries in SLADE, such as in zip/pk3 archives.

## Properties

| Name | Type | Description |
|:-----|:-----|:------------|
`name` | `string` | The directory name
`archive` | <code>[Archive](Archive.md)</code> | The <code>[Archive](Archive.md)</code> the directory is a part of
`entries` | <code>[ArchiveEntry](ArchiveEntry.md)\[\]</code> | An array of <code>[ArchiveEntry](ArchiveEntry.md)</code> objects contained in the directory. Does not include entries within subdirectories
`parent` | `ArchiveDir` | The `ArchiveDir` the directory is contained in. This will be `null` if the directory has no parent
`path` | `string` | The full path of the directory, including itself

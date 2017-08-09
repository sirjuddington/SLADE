The **ArchiveDir** type represents a 'directory' of entries in SLADE, such as in zip/pk3 archives.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>name</prop> | <type>string</type> | The directory name
<prop>archive</prop> | <type>[Archive](Archive.md)</type> | The <type>[Archive](Archive.md)</type> the directory is a part of
<prop>entries</prop> | <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type> | An array of <type>[ArchiveEntry](ArchiveEntry.md)</type> objects contained in the directory. Does not include entries within subdirectories
<prop>parent</prop> | <type>ArchiveDir</type> | The <type>ArchiveDir</type> the directory is contained in. This will be `nil` if the directory has no parent
<prop>path</prop> | <type>string</type> | The full path of the directory, including itself
<prop>subDirectories</prop> | <type>[ArchiveDir](ArchiveDir.md)\[\]</type> | An array of all subdirectories within the directory

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

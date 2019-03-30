<article-head>ArchiveDir</article-head>

The <type>ArchiveDir</type> type represents a 'directory' of entries in SLADE, such as in zip/pk3 archives.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">name</prop> | <type>string</type> | The directory name
<prop class="ro">archive</prop> | <type>[Archive](Archive.md)</type> | The <type>[Archive](Archive.md)</type> the directory is a part of
<prop class="ro">entries</prop> | <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type> | An array of <type>[ArchiveEntry](ArchiveEntry.md)</type> objects contained in the directory. Does not include entries within subdirectories
<prop class="ro">parent</prop> | <type>ArchiveDir</type> | The <type>ArchiveDir</type> the directory is contained in. This will be `nil` if the directory has no parent
<prop class="ro">path</prop> | <type>string</type> | The full path of the directory, including itself
<prop class="ro">subDirectories</prop> | <type>[ArchiveDir](ArchiveDir.md)\[\]</type> | An array of all subdirectories within the directory

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[Archive.DirAtPath](Archive.md#diratpath)</code>

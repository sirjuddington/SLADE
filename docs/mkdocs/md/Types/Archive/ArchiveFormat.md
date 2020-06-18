<subhead>Type</subhead>
<header>ArchiveFormat</header>

The <type>ArchiveFormat</type> type contains information about an archive's format.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">id</prop> | <type>string</type> | The format id
<prop class="ro">name</prop> | <type>string</type> | The format name
<prop class="ro">supportsDirs</prop> | <type>boolean</type> | Whether the archive format supports directories
<prop class="ro">hasExtensions</prop> | <type>boolean</type> | Whether entry names in the archive have extensions
<prop class="ro">maxNameLength</prop> | <type>integer</type> | The maximum length of entry names

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Archive.format](Archive.md#properties)</code>

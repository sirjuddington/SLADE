<subhead>Type</subhead>
<header>ArchiveSearchOptions</header>

The <type>ArchiveSearchOptions</type> type contains options for searching for entries in an <type>[Archive](Archive.md)</type>.

**See:**

* <code>[Archive.FindFirst](Archive.md#findfirst)</code>
* <code>[Archive.FindLast](Archive.md#findlast)</code>
* <code>[Archive.FindAll](Archive.md#findall)</code>

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">matchName</prop> | <type>string</type> | Search for entries with names matching this string. Can contain `*` or `?` wildcards, eg. `D_*`. Ignored if empty
<prop class="rw">matchType</prop> | <type>[EntryType](EntryType.md)</type> | Search for entries of this type. Ignored if `nil`
<prop class="rw">matchNamespace</prop> | <type>string</type> | Search for entries only within this namespace. Ignored if empty
<prop class="rw">dir</prop> | <type>[ArchiveDir](ArchiveDir.md)</type> | Search for entries only within this dir. If `nil`, the root directory of the archive is searched
<prop class="rw">ignoreExt</prop> | <type>boolean</type> | If `true`, the extension of the entry name is ignored when checking for a match with <prop>matchName</prop>
<prop class="rw">searchSubdirs</prop> | <type>boolean</type> | If `true`, subdirectories within <prop>dir</prop> will also be searched recursively

## Constructors

<code><type>ArchiveSearchOptions</type>.<func>new</func>()</code>

Creates a new <type>ArchiveSearchOptions</type> with the following default values:

* <prop>matchName</prop>, <prop>matchNamespace</prop> as empty strings
* <prop>matchType</prop>, <prop>dir</prop> as `nil`
* <prop>ignoreExt</prop> `true`
* <prop>searchSubdirs</prop> `false`

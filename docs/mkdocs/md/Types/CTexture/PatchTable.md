<subhead>Type</subhead>
<header>PatchTable</header>

A patch table is a list of patch names used for Doom's composite texture system (see: [PNAMES](https://doomwiki.org/wiki/PNAMES) on the doom wiki).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">patches</prop> | <type>string[]</type> | An array of all patch names in the table
<prop class="rw">parent</prop> | <type>[Archive](../Archive/Archive.md)</type> | The parent archive of the patch table

## Constructors

<code><type>PatchTable</type>.<func>new</func>()</code>

Creates a new, empty patch table.

## Functions

### Overview

#### Patches

<fdef>[Patch](#patch)(<arg>index</arg>) -> <type>string</type></fdef>
<fdef>[PatchEntry](#patchentry-1)(<arg>index</arg>) -> <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type></fdef>
<fdef>[PatchEntry](#patchentry-2)(<arg>name</arg>) -> <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type></fdef>
<fdef>[RemovePatch](#removepatch)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[ReplacePatch](#replacepatch)(<arg>index</arg>, <arg>name</arg>) -> <type>boolean</type></fdef>
<fdef>[AddPatch](#addpatch)(<arg>name</arg>, <arg>allowDuplicate</arg>) -> <type>boolean</type></fdef>

#### Read/Write

<fdef>[LoadPNAMES](#loadpnames)(<arg>pnames</arg>) -> <type>boolean</type></fdef>
<fdef>[WritePNAMES](#writepnames)(<arg>pnames</arg>) -> <type>boolean</type></fdef>

---
### Patch

Gets the name of the patch at <arg>index</arg>.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The patch index

#### Returns

* <type>string</type>: The name of the patch at <arg>index</arg>, or `"INVALID PATCH"` if the index was out of bounds

---
### PatchEntry <sup>(1)</sup>

Gets the associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch at <arg>index</arg> in the list.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The patch index

#### Returns

* <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>: The associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch at <arg>index</arg>, or `nil` if no entry was found or the given index was out of bounds

#### Notes

When searching for the patch entry, the <prop>parent</prop> archive of the table will be prioritised.

---
### PatchEntry <sup>(2)</sup>

Gets the associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch with the given <arg>name</arg> in the list.

#### Parameters

* <arg>name</arg> (<type>string</type>): The patch name

#### Returns

* <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> The associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch with the given <arg>name</arg>, or `nil` if no entry was found or the given name was not in the patch table

#### Notes

When searching for the patch entry, the <prop>parent</prop> archive of the table will be prioritised.

---
### RemovePatch

Removes the patch at <arg>index</arg> from the table.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The patch index

#### Returns

* <type>boolean</type>: `false` if the given index was out of bounds

---
### ReplacePatch

Replaces the patch at <arg>index</arg> in the table with <arg>name</arg>.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The patch index
* <arg>name</arg> (<type>string</type>): The new name for the patch

#### Returns

* <type>boolean</type>: `false` if the given index was out of bounds

---
### AddPatch

Adds a new patch with <arg>name</arg> to the end of the table. 

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the patch to add
* <arg>allowDuplicate</arg> (<type>boolean</type>): If `false`, no new patch will be added if a patch already exists in the table with the given name

#### Returns

* <type>boolean</type>: `true` if a new patch was added

---
### LoadPNAMES

Clears any existing patches in the table and loads in PNAMES data from the given entry <arg>pnames</arg>.

#### Parameters

* <arg>pnames</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The PNAMES entry to load

#### Returns

* <type>boolean</type>: `true` on success

#### Notes

This will set the table's <prop>parent</prop> property to the parent archive of the given <arg>pnames</arg> entry.

---
### WritePNAMES

Writes the patch table to <arg>pnames</arg> in PNAMES format.

#### Parameters

* <arg>pnames</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to write PNAMES data to

#### Returns

* <type>boolean</type>: `true` on success

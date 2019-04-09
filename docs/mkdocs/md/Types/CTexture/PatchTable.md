<article-head>PatchTable</article-head>

A patch table is a list of patch names used for Doom's composite texture system (see: [PNAMES](https://doomwiki.org/wiki/PNAMES) on the doom wiki).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">patches</prop> | <type>string[]</type> | An array of all patch names in the table
<prop class="rw">parent</prop> | <type>[Archive](../Archive/Archive.md)</type> | The parent archive of the patch table

## Constructors

<fdef>function <type>PatchTable</type>.<func>new</func>()</fdef>

Creates a new, empty patch table.

## Functions

### Patch

<fdef>function <type>PatchTable</type>.<func>Patch</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The patch index

<listhead>Returns</listhead>

* <type>string</type>: The name of the patch at <arg>index</arg>, or `"INVALID PATCH"` if the index was out of bounds

---
### PatchEntry <sup>(1)</sup>

<fdef>function <type>PatchTable</type>.<func>PatchEntry</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The patch index

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>: The associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch at <arg>index</arg>, or `nil` if no entry was found or the given index was out of bounds

#### Notes

When searching for the patch entry, the <prop>parent</prop> archive of the table will be prioritised.

---
### PatchEntry <sup>(2)</sup>

<fdef>function <type>PatchTable</type>.<func>PatchEntry</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The patch name

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> The associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch with the given <arg>name</arg>, or `nil` if no entry was found or the given name was not in the patch table

#### Notes

When searching for the patch entry, the <prop>parent</prop> archive of the table will be prioritised.

---
### RemovePatch

<fdef>function <type>PatchTable</type>.<func>RemovePatch</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

Removes the patch at <arg>index</arg> from the table.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The patch index

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given index was out of bounds

---
### ReplacePatch

<fdef>function <type>PatchTable</type>.<func>ReplacePatch</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>name</arg>)</fdef>

Replaces the patch at <arg>index</arg> in the table with <arg>name</arg>.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The patch index
* <arg>name</arg> (<type>string</type>): The new name for the patch

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given index was out of bounds

---
### AddPatch

<fdef>function <type>PatchTable</type>.<func>AddPatch</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>allowDuplicate</arg>)</fdef>

Adds a new patch with <arg>name</arg> to the end of the table. 

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the patch to add
* <arg>allowDuplicate</arg> (<type>boolean</type>): If `false`, no new patch will be added if a patch already exists in the table with the given name

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if a new patch was added

---
### LoadPNAMES

<fdef>function <type>PatchTable</type>.<func>LoadPNAMES</func>(<arg>*self*</arg>, <arg>pnames</arg>)</fdef>

Clears any existing patches in the table and loads in PNAMES data from the given entry <arg>pnames</arg>.

<listhead>Parameters</listhead>

* <arg>pnames</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The PNAMES entry to load

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

#### Notes

This will set the table's <prop>parent</prop> property to the parent archive of the given <arg>pnames</arg> entry.

---
### WritePNAMES

<fdef>function <type>PatchTable</type>.<func>WritePNAMES</func>(<arg>*self*</arg>, <arg>pnames</arg>)</fdef>

Writes the patch table to <arg>pnames</arg> in PNAMES format.

<listhead>Parameters</listhead>

* <arg>pnames</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to write PNAMES data to

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

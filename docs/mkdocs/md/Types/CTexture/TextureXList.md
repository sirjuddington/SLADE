The <type>TextureXList</type> type is a list of composite textures (<type>[CTexture](CTexture.md)</type>), which can be in various formats.

## Constants

| Name | Value | Description |
|:-----|:------|:------------|
`FORMAT_NORMAL` | 0 | Doom TEXTUREx format
`FORMAT_STRIFE11` | 1 | Strife TEXTUREx format
`FORMAT_NAMELESS` | 2 | Doom Alpha nameless format
`FORMAT_TEXTURES` | 3 | ZDoom TEXTURES format
`FORMAT_JAGUAR` | 4 | Jaguar Doom TEXTUREx format

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">textures</prop> | <type>[CTexture](CTexture.md)\[\]</type> | All textures in the list
<prop class="ro">format</prop> | <type>number</type> | The format of this list (see `FORMAT_` constants)
<prop class="ro">formatString</prop> | <type>string</type> | The name of the list <prop>format</prop>

## Constructors

<code><type>TextureXList</type>.<func>new</func>()</code>

Creates a new, empty list with <prop>format</prop> set to `FORMAT_NORMAL`.

---

<code><type>TextureXList</type>.<func>new</func>(<arg>format</arg>)</code>

Creates a new, empty list of the given <arg>format</arg>.

#### Parameters

* <arg>format</arg> (<type>number</type>): The format of the list (see `FORMAT_` constants)


## Functions

### Overview

#### General

<fdef>[Texture](#texture)(<arg>name</arg>) -> <type>[CTexture](CTexture.md)</type></fdef>
<fdef>[TextureIndex](#textureindex)(<arg>name</arg>) -> <type>number</type></fdef>
<fdef>[ConvertToTEXTURES](#converttotextures)() -> <type>boolean</type></fdef>
<fdef>[FindErrors](#finderrors)() -> <type>boolean</type></fdef>

#### Texture List Modification

<fdef>[AddTexture](#addtexture)(<arg>name</arg>, <arg>[extended]</arg>, <arg>[position]</arg>) -> <type>[CTexture](CTexture.md)</type></fdef>
<fdef>[RemoveTexture](#removetexture)(<arg>position</arg>)</fdef>
<fdef>[SwapTextures](#swaptextures)(<arg>position1</arg>, <arg>position2</arg>)</fdef>
<fdef>[Clear](#clear)()</fdef>
<fdef>[RemovePatch](#removepatch)(<arg>patch</arg>)</fdef>

#### Read/Write

<fdef>[ReadTEXTUREXData](#readtexturexdata)(<arg>entry</arg>, <arg>patchTable</arg>, <arg>additive</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteTEXTUREXData](#writetexturexdata)(<arg>entry</arg>, <arg>patchTable</arg>) -> <type>boolean</type></fdef>
<fdef>[ReadTEXTURESData](#readtexturesdata)(<arg>entry</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteTEXTURESData](#writetexturesdata)(<arg>entry</arg>) -> <type>boolean</type></fdef>

---
### Texture

Gets the texture in the list matching <arg>name</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the texture to get

#### Returns

* <type>[CTexture](CTexture.md)</type>: The first texture found in the list with <arg>name</arg>, or `nil` if not found

---
### TextureIndex

Gets the index of the texture in the list matching <arg>name</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the texture

#### Returns

* <type>number</type>: The index of the first texture found in the list with <arg>name</arg>, or `0` if not found

---
### ConvertToTEXTURES

Converts all textures in the list to extended format.

#### Returns

* <type>boolean</type>: `true` on success, `false` if the list was already in `TEXTURES` format

#### Notes

This will set <prop>format</prop> to `FORMAT_TEXTURES`.

---
### FindErrors

Finds and logs any errors in the texture list.

#### Returns

* <type>boolean</type>: `true` if any errors were found

---
### AddTexture

Adds a new texture to the list.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the new texture
* <arg>[extended]</arg> (<type>boolean</type>): If `true`, the new texture will be in ZDoom `TEXTURES` format. Default is `false`
* <arg>[position]</arg> (<type>number</type>): The position the new texture should be added in the list. Default is `0`, which will add to the end of the list

#### Returns

* <type>[CTexture](CTexture.md)</type>: The newly created texture

---
### RemoveTexture

Removes the texture at <arg>position</arg> in the list.

#### Parameters

* <arg>position</arg> (<type>number</type>): The position of the texture to remove

---
### SwapTextures

Swaps the textures at <arg>position1</arg> and <arg>position2</arg> in the list.

#### Parameters

* <arg>position1</arg> (<type>number</type>): The position of the first texture to swap
* <arg>position2</arg> (<type>number</type>): The position of the second texture to swap

---
### Clear

Removes all textures from the list.

---
### RemovePatch

Removes <arg>patch</arg> from all textures in the list.

#### Parameters

* <arg>patch</arg> (<type>string</type>): The name of the patch to be removed

---
### ReadTEXTUREXData

Reads a Doom-format `TEXTUREx` entry.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to read the data from
* <arg>patchTable</arg> (<type>[PatchTable](PatchTable.md)</type>): The patch table (PNAMES) to use for patch names
* <arg>additive</arg> (<type>boolean</type>): If `true`, textures are added to the current list, otherwise all textures are cleared first

#### Returns

* <type>boolean</type>: `true` on success

#### Notes

This will set <prop>format</prop> to the type of list that was loaded.

---
### WriteTEXTUREXData

Writes the list in Doom `TEXTUREx` format to <arg>entry</arg>.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to write the data to
* <arg>patchTable</arg> (<type>[PatchTable](PatchTable.md)</type>): The patch table (PNAMES) to use for patch names

#### Returns

* <type>boolean</type>: `true` on success

---
### ReadTEXTURESData

Reads ZDoom `TEXTURES` data from <arg>entry</arg>.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to read data from

#### Returns

* <type>boolean</type>: `true` on success

---
### WriteTEXTURESData

Writes the list in ZDoom `TEXURES` format to <arg>entry</arg>.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to write data to

#### Returns

* <type>boolean</type>: `true` on success

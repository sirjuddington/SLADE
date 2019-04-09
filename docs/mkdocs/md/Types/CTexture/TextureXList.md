<article-head>TextureXList</article-head>

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

<fdef>function <type>TextureXList</type>.<func>new</func>()</fdef>

Creates a new, empty list with <prop>format</prop> set to `FORMAT_NORMAL`.

---

<fdef>function <type>TextureXList</type>.<func>new</func>(<arg>format</arg>)</fdef>

Creates a new, empty list of the given <arg>format</arg>.

Parameters

* <arg>format</arg> (<type>number</type>): The format of the list (see `FORMAT_` constants)


## Functions - General

### Texture

<fdef>function <type>TextureXList</type>.<func>Texture</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the texture to get

<listhead>Returns</listhead>

* <type>[CTexture](CTexture.md)</type>: The first texture found in the list with <arg>name</arg>, or `nil` if not found

---
### TextureIndex

<fdef>function <type>TextureXList</type>.<func>TextureIndex</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the texture

<listhead>Returns</listhead>

* <type>number</type>: The index of the first texture found in the list with <arg>name</arg>, or `0` if not found

---
### ConvertToTEXTURES

<fdef>function <type>TextureXList</type>.<func>ConvertToTEXTURES</func>(<arg>*self*</arg>)</fdef>

Converts all textures in the list to extended format.

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success, `false` if the list was already in `TEXTURES` format

#### Notes

This will set <prop>format</prop> to `FORMAT_TEXTURES`.

---
### FindErrors

<fdef>function <type>TextureXList</type>.<func>FindErrors</func>(<arg>*self*</arg>)</fdef>

Finds and logs any errors in the texture list.

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if any errors were found


## Functions - Texture List Modification

### AddTexture

<!--
<fdef>
<block>function <type>TextureXList</type>.<func>AddTexture</func>(</block>
<block><arg>*self*</arg>,<br/><arg>name</arg>,<br/><arg>extended</arg>,<br/><arg>index</arg></block>
<block><br/><br/><br/>)</block>
<block><br/>-- <type>string</type><br/>-- <type>boolean</type><br/>-- <type>number</type></block>
<block><br/><br/>(default: false)<br/>(default: 0)</block>
</fdef>
-->
<fdef>function <type>TextureXList</type>.<func>AddTexture</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>extended</arg>, <arg>position</arg>)</fdef>

Adds a new texture to the list.

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the new texture
* <arg>[extended]</arg> (<type>boolean</type>, default `false`): If `true`, the new texture will be in ZDoom `TEXTURES` format
* <arg>[position]</arg> (<type>number</type>, default `0`): The position the new texture should be added in the list. If `0`, it will be added to the end of the list

<listhead>Returns</listhead>

* <type>[CTexture](CTexture.md)</type>: The newly created texture

---
### RemoveTexture

<fdef>function <type>TextureXList</type>.<func>RemoveTexture</func>(<arg>*self*</arg>, <arg>position</arg>)</fdef>

Removes the texture at <arg>position</arg> in the list.

<listhead>Parameters</listhead>

* <arg>position</arg> (<type>number</type>): The position of the texture to remove

---
### SwapTextures

<fdef>function <type>TextureXList</type>.<func>SwapTextures</func>(<arg>*self*</arg>, <arg>position1</arg>, <arg>position2</arg>)</fdef>

Swaps the textures at <arg>position1</arg> and <arg>position2</arg> in the list.

<listhead>Parameters</listhead>

* <arg>position1</arg> (<type>number</type>): The position of the first texture to swap
* <arg>position2</arg> (<type>number</type>): The position of the second texture to swap

---
### Clear

<fdef>function <type>TextureXList</type>.<func>Clear</func>(<arg>*self*</arg>)</fdef>

Removes all textures from the list.

---
### RemovePatch

<fdef>function <type>TextureXList</type>.<func>RemovePatch</func>(<arg>*self*</arg>, <arg>patch</arg>)</fdef>

Removes <arg>patch</arg> from all textures in the list.

<listhead>Parameters</listhead>

* <arg>patch</arg> (<type>string</type>): The name of the patch to be removed


## Functions - Read/Write

### ReadTEXTUREXData

<fdef>function <type>TextureXList</type>.<func>ReadTEXTUREXData</func>(<arg>*self*</arg>, <arg>entry</arg>, <arg>patchTable</arg>, <arg>additive</arg>)</fdef>

Reads a Doom-format `TEXTUREx` entry.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to read the data from
* <arg>patchTable</arg> (<type>[PatchTable](PatchTable.md)</type>): The patch table (PNAMES) to use for patch names
* <arg>additive</arg> (<type>boolean</type>): If `true`, textures are added to the current list, otherwise all textures are cleared first

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

#### Notes

This will set <prop>format</prop> to the type of list that was loaded.

---
### WriteTEXTUREXData

<fdef>function <type>TextureXList</type>.<func>WriteTEXTUREXData</func>(<arg>*self*</arg>, <arg>entry</arg>, <arg>patchTable</arg>)</fdef>

Writes the list in Doom `TEXTUREx` format to <arg>entry</arg>.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to write the data to
* <arg>patchTable</arg> (<type>[PatchTable](PatchTable.md)</type>): The patch table (PNAMES) to use for patch names

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### ReadTEXTURESData

<fdef>function <type>TextureXList</type>.<func>ReadTEXTURESData</func>(<arg>*self*</arg>, <arg>entry</arg>)</fdef>

Reads ZDoom `TEXTURES` data from <arg>entry</arg>.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to read data from

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### WriteTEXTURESData

<fdef>function <type>TextureXList</type>.<func>WriteTEXTURESData</func>(<arg>*self*</arg>, <arg>entry</arg>)</fdef>

Writes the list in ZDoom `TEXURES` format to <arg>entry</arg>.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to write data to

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

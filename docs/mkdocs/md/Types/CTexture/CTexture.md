<subhead>Type</subhead>
<header>CTexture</header>

Represents a (z)doom format composite texture. A composite texture is made up of one or more patches (<type>[CTPatch](CTPatch.md)</type>).

For more information, see:

* [TEXTURE1 and TEXTURE2](https://doomwiki.org/wiki/TEXTURE1_and_TEXTURE2) for Doom format `TEXTUREx`
* [TEXTURES](https://zdoom.org/wiki/TEXTURES) for ZDoom format `TEXTURES`


## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">patches</prop> | <type>[CTPatch](CTPatch.md)\[\]</type> | The patches that make up the texture
<prop class="rw">name</prop> | <type>string</type> | The texture name
<prop class="rw">width</prop> | <type>integer</type> | Width of the texture
<prop class="rw">height</prop> | <type>integer</type> | Height of the texture
<prop class="rw">worldPanning</prop> | <type>boolean</type> | If `true`, use world units instead of pixels for offsets (in map)
<prop class="rw">extended</prop> | <type>boolean</type> | If `true`, this is a ZDoom `TEXTURES` format texture, and contains <type>[CTPatchEx](CTPatchEx.md)</type> patches
<prop class="rw">type</prop> | <type>string</type> | The type of the texture: `texture`, `sprite`, `graphic`, `walltexture` or `flat`
<prop class="rw">scaleX</prop> | <type>float</type> | Horizontal scale of the texture (multiplier, eg. `0.5` is half size)
<prop class="rw">scaleY</prop> | <type>float</type> | Vertical scale of the texture (multiplier, eg. `0.5` is half size)
<prop class="rw">offsetX</prop> | <type>integer</type> | X offset of the texture
<prop class="rw">offsetY</prop> | <type>integer</type> | Y offset of the texture
<prop class="rw">optional</prop> | <type>boolean</type> | If `true`, texture is optional
<prop class="rw">noDecals</prop> | <type>boolean</type> | If `true`, no decals will show on this texture
<prop class="rw">nullTexture</prop> | <type>boolean</type> | If `true`, this texture is never drawn ingame (like `AASHITTY`)


## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[TextureXList.AddTexture](TextureXList.md#addtexture)</code>


## Functions

### Overview

#### General

<fdef>[AsText](#astext)() -> <type>string</type></fdef>
<fdef>[Clear](#clear)()</fdef>
<fdef>[ConvertExtended](#convertextended)()</fdef>
<fdef>[ConvertRegular](#convertregular)()</fdef>
<fdef>[CopyTexture](#copytexture)(<arg>other</arg>, <arg>[keepFormat]</arg>)</fdef>

#### Patch Modification

<fdef>[AddPatch](#addpatch)(<arg>patch</arg>, <arg>[x]</arg>, <arg>[y]</arg>, <arg>[index]</arg>)</fdef>
<fdef>[DuplicatePatch](#duplicatepatch)(<arg>index</arg>, <arg>[offsetX]</arg>, <arg>[offsetY]</arg>)</fdef>
<fdef>[RemovePatch](#removepatch)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[ReplacePatch](#replacepatch)(<arg>index</arg>, <arg>newPatch</arg>) -> <type>boolean</type></fdef>
<fdef>[SwapPatches](#swappatches)(<arg>index1</arg>, <arg>index2</arg>) -> <type>boolean</type></fdef>

---
### AsText

Gets the text definition of the texture in ZDoom `TEXTURES` format.

#### Returns

* <type>string</type>: A text representation of the texture

---
### Clear

Clears all texture data.

---
### ConvertExtended

Converts the texture to an 'extended' (ZDoom `TEXTURES`) texture, and all patches to <type>[CTPatchEx](CTPatchEx.md)</type>.

Does nothing if the texture is already extended format (<prop>extended</prop> = `true`).

---
### ConvertRegular

Converts the texture to an 'regular' (Doom `TEXTUREx`) texture, and all patches to <type>[CTPatch](CTPatch.md)</type>.

Does nothing if the texture is already regular format (<prop>extended</prop> = `false`).

---
### CopyTexture

Copies another texture.

#### Parameters

* <arg>other</arg> (<type>CTexture</type>): The texture to copy
* <arg>[keepFormat]</arg> (<type>boolean</type>): If `true`, the current texture format (<prop>extended</prop> property) will be kept, otherwise it will be converted to the format of <arg>other</arg>. Default is `false`

---
### AddPatch

Adds a new patch to the texture.

#### Parameters

* <arg>patch</arg> (<type>string</type>): The name of the patch to add
* <arg>[x]</arg> (<type>integer</type>, default `0`): The x position of the patch within the texture
* <arg>[y]</arg> (<type>integer</type>, default `0`): The y position of the patch within the texture
* <arg>[index]</arg> (<type>integer</type>, default `-1`): Where to add the patch in the <prop>patches</prop> array. Patches later in the array are drawn over the top of previous ones. If `-1`, the patch is added to the end of the array

#### Notes

The type of patch created depends on this texture's <prop>extended</prop> property:

* `false`: <type>[CTPatch](CTPatch.md)</type>
* `true`: <type>[CTPatchEx](CTPatchEx.md)</type>

---
### DuplicatePatch

Duplicates the patch at <arg>index</arg> in the texture.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The index of the patch to duplicate
* <arg>[offsetX]</arg> (<type>integer</type>): The amount to add to the X offset of the duplicated patch. Default is `8`
* <arg>[offsetY]</arg> (<type>integer</type>): The amount to add to the Y offset of the duplicated patch. Default is `8`

#### Example

```lua
-- Add Patch
texture:AddPatch('patch1', 10, 10)

-- Duplicate Patch
texture:DuplicatePatch(1, 5, 5)

local dupPatch = texture.patches[2]
App.LogMessage('@' .. dupPatch.offsetX .. ',' .. dupPatch.offsetY) -- @15,15
```

---
### RemovePatch

Removes the patch at <arg>index</arg> in the texture.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The index of the patch to remove

#### Returns

* <type>boolean</type>: `false` if the given <arg>index</arg> was invalid (no patch removed)

---
### ReplacePatch

Replaces the patch at <arg>index</arg> in the texture with <arg>newPatch</arg>.

#### Parameters

* <arg>index</arg> (<type>integer</type>): The index of the patch to replace
* <arg>newPatch</arg> (<type>string</type>): The new patch name

#### Returns

* <type>boolean</type>: `false` if the given <arg>index</arg> was invalid (no patch replaced)

---
### SwapPatches

Swaps the patch at <arg>index1</arg> in the texture with the patch at <arg>index2</arg>.

#### Parameters

* <arg>index1</arg> (<type>integer</type>): The index of the first patch to swap
* <arg>index2</arg> (<type>integer</type>): The index of the second patch to swap

#### Returns

* <type>boolean</type>: `false` if either of the given indices were invalid (no patches swapped)

<article-head>CTexture</article-head>

Represents a (z)doom format composite texture. A composite texture is made up of one or more patches (<type>[CTPatch](CTPatch.md)</type>).

For more information, see:

* [TEXTURE1 and TEXTURE2](https://doomwiki.org/wiki/TEXTURE1_and_TEXTURE2) for Doom format `TEXTUREx`
* [TEXTURES](https://zdoom.org/wiki/TEXTURES) for ZDoom format `TEXTURES`


## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">patches</prop> | <type>[CTPatch](CTPatch.md)\[\]</type> | The patches that make up the texture
<prop class="rw">name</prop> | <type><type>string</type> | The texture name
<prop class="rw">width</prop> | <type><type>number</type> | Width of the texture
<prop class="rw">height</prop> | <type><type>number</type> | Height of the texture
<prop class="rw">worldPanning</prop> | <type><type>boolean</type> | If `true`, use world units instead of pixels for offsets (in map)
<prop class="rw">extended</prop> | <type><type>boolean</type> | If `true`, this is a ZDoom `TEXTURES` format texture, and contains <type>[CTPatchEx](CTPatchEx.md)</type> patches
<prop class="rw">type</prop> | <type><type>string</type> | The type of the texture: `texture`, `sprite`, `graphic`, `walltexture` or `flat`
<prop class="rw">scaleX</prop> | <type><type>number</type> | Horizontal scale of the texture (multiplier, eg. `0.5` is half size)
<prop class="rw">scaleY</prop> | <type><type>number</type> | Vertical scale of the texture (multiplier, eg. `0.5` is half size)
<prop class="rw">offsetX</prop> | <type><type>number</type> | X offset of the texture
<prop class="rw">offsetY</prop> | <type><type>number</type> | Y offset of the texture
<prop class="rw">optional</prop> | <type><type>boolean</type> | If `true`, texture is optional
<prop class="rw">noDecals</prop> | <type><type>boolean</type> | If `true`, no decals will show on this texture
<prop class="rw">nullTexture</prop> | <type><type>boolean</type> | If `true`, this texture is never drawn ingame (like `AASHITTY`)


## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* [TextureXList.AddTexture](TextureXList.md#addtexture)


## Functions - General

### AsText

<fdef>function <type>CTexture</type>.<func>AsText</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>string</type>: A text representation of the texture in ZDoom `TEXTURES` format

---
### Clear

<fdef>function <type>CTexture</type>.<func>Clear</func>(<arg>*self*</arg>)</fdef>

Clears all texture data.

---
### ConvertExtended

<fdef>function <type>CTexture</type>.<func>ConvertExtended</func>(<arg>*self*</arg>)</fdef>

Converts the texture to an 'extended' (ZDoom `TEXTURES`) texture, and all patches to <type>[CTPatchEx](CTPatchEx.md)</type>.

Does nothing if the texture is already extended format (<prop>extended</prop> = `true`).

---
### ConvertRegular

<fdef>function <type>CTexture</type>.<func>ConvertRegular</func>(<arg>*self*</arg>)</fdef>

Converts the texture to an 'regular' (Doom `TEXTUREx`) texture, and all patches to <type>[CTPatch](CTPatch.md)</type>.

Does nothing if the texture is already regular format (<prop>extended</prop> = `false`).

---
### CopyTexture

<fdef>function <type>CTexture</type>.<func>CopyTexture</func>(<arg>*self*</arg>, <arg>other</arg>, <arg>keepFormat</arg>)</fdef>

Copies another texture.

<listhead>Parameters</listhead>

* <arg>other</arg> (<type>CTexture</type>): The texture to copy
* <arg>[keepFormat]</arg> (<type>boolean</type>, default `false`): If `true`, the current texture format (<prop>extended</prop> property) will be kept, otherwise it will be converted to the format of <arg>other</arg>


## Functions - Patch Modification

### AddPatch

<fdef>function <type>CTexture</type>.<func>AddPatch</func>(<arg>*self*</arg>, <arg>patch</arg>, <arg>x</arg>, <arg>y</arg>, <arg>index</arg>)</fdef>

Adds a new patch to the texture.

<listhead>Parameters</listhead>

* <arg>patch</arg> (<type>string</type>): The name of the patch to add
* <arg>[x]</arg> (<type>number</type>, default `0`): The x position of the patch within the texture
* <arg>[y]</arg> (<type>number</type>, default `0`): The y position of the patch within the texture
* <arg>[index]</arg> (<type>number</type>, default `0`): Where to add the patch in the <prop>patches</prop> array. Patches later in the array are drawn over the top of previous ones. If `0`, the patch is added to the end of the array

**Notes**

The type of patch created depends on this texture's <prop>extended</prop> property:

* `false`: <type>[CTPatch](CTPatch.md)</type>
* `true`: <type>[CTPatchEx](CTPatchEx.md)</type>

---
### DuplicatePatch

<fdef>function <type>CTexture</type>.<func>DuplicatePatch</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>offsetX</arg>, <arg>offsetY</arg>)</fdef>

Duplicates the patch at <arg>index</arg> in the texture.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The index of the patch to duplicate
* <arg>[offsetX]</arg> (<type>number</type>, default `8`): The amount to add to the X offset of the duplicated patch
* <arg>[offsetY]</arg> (<type>number</type>, default `8`): The amount to add to the Y offset of the duplicated patch

**Example**

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

<fdef>function <type>CTexture</type>.<func>RemovePatch</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

Removes the patch at <arg>index</arg> in the texture.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The index of the patch to remove

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given <arg>index</arg> was invalid (no patch removed)

---
### ReplacePatch

<fdef>function <type>CTexture</type>.<func>ReplacePatch</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>newPatch</arg>)</fdef>

Replaces the patch at <arg>index</arg> in the texture with <arg>newPatch</arg>.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The index of the patch to replace
* <arg>newPatch</arg> (<type>string</type>): The new patch name

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given <arg>index</arg> was invalid (no patch replaced)

---
### SwapPatches

<fdef>function <type>CTexture</type>.<func>SwapPatches</func>(<arg>*self*</arg>, <arg>index1</arg>, <arg>index2</arg>)</fdef>

Swaps the patch at <arg>index1</arg> in the texture with the patch at <arg>index2</arg>.

<listhead>Parameters</listhead>

* <arg>index1</arg> (<type>number</type>): The index of the first patch to swap
* <arg>index2</arg> (<type>number</type>): The index of the second patch to swap

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if either of the given indices were invalid (no patches swapped)

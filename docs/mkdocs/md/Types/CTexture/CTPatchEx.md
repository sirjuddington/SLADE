<subhead>Type</subhead>
<header>CTPatchEx</header>

An extended composite texture patch, with extra properties for use in ZDoom's [TEXTURES](https://zdoom.org/wiki/TEXTURES) lump.

### Inherits <type>[CTPatch](CTPatch.md)</type>  
All properties and functions of <type>[CTPatch](CTPatch.md)</type> can be used in addition to those below.

## Constants

| Name | Value |
|:-----|:------|
`TYPE_PATCH` | 0
`TYPE_GRAPHIC` | 1
`BLENDTYPE_NONE` | 0
`BLENDTYPE_TRANSLATION` | 1
`BLENDTYPE_BLEND` | 2
`BLENDTYPE_TINT` | 3

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">flipX</prop> | <type>boolean</type> | If `true`, the patch will be flipped horizontally
<prop class="rw">flipY</prop> | <type>boolean</type> | If `true`, the patch will be flipped vertically
<prop class="rw">useOffsets</prop> | <type>boolean</type> | If `true`, the patch graphic's offsets will be used in addition to the <prop>offsetX</prop> and <prop>offsetY</prop> properties (see <type>[CTPatch](CTPatch.md)</type>)
<prop class="rw">rotation</prop> | <type>integer</type> | The rotation of the patch in degrees. Only supports right angles (0, 90, 180, 270)
<prop class="rw">colour</prop> | <type>[Colour](../Colour.md)</type> | The blend colour to use. Only applicable if <prop>blendType</prop> is `Blend` or `Tint`
<prop class="rw">alpha</prop> | <type>float</type> | The translucency of the patch, from 0.0 (invisible) to 1.0 (opaque)
<prop class="rw">style</prop> | <type>string</type> | The style of translucency to use, if <prop>alpha</prop> is < 1.0
<prop class="rw">blendType</prop> | <type>integer</type> | The type of colour blend to use for this patch (see `BLENDTYPE_` constants). This combines the `Translation` and `Blend` properties for ZDoom TEXTURES patches, since they are mutually exclusive. If the blend type is set to `BLENDTYPE_TINT`, the alpha value of <prop>colour</prop> is used to specify the tint amount
<prop class="rw">translation</prop> | <type>[Translation](../Translation/Translation.md)</type> | The colour translation to use if <prop>blendType</prop> is set to `Translation`

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[CTexture.AddPatch](CTexture.md#addpatch)</code>

## Functions

### Overview

<fdef>[AsText](#astext)()</fdef>

---
### AsText

Gets the ZDoom [TEXTURES](https://zdoom.org/wiki/TEXTURES) format text definition for this patch.

#### Returns

* <type>string</type>: A text representation of the patch

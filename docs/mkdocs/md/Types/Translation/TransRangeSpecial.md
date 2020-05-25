A custom translation range that maps to a built-in colour translation.

!!! attention
    Note that this range type isn't valid in ZDoom, it is used internally in SLADE only. Only use this when applying a translation to a graphic or palette in SLADE.

### Inherits <type>[TransRange](TransRange.md)</type>  
All properties and functions of <type>[TransRange](TransRange.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">special</prop> | <type>string</type> | The name of the built in translation, can be: `ice`, `inverse`, `red`, `green`, `blue`, `gold` or `desat##` where `##` is a number from `1` to `31` (see [TEXTURES](https://zdoom.org/wiki/TEXTURES))

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Translation.AddSpecialRange](Translation.md#addspecialrange)</code>

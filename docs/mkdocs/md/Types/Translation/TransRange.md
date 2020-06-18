<subhead>Type</subhead>
<header>TransRange</header>

A single 'part' of a custom <type>[Translation](Translation.md)</type>, which can be made up of multiple (each one corresponding to a range in the palette).

### Derived Types

The following types inherit all <type>TransRange</type> properties and functions:

* <type>[TransRangePalette](TransRangePalette.md)</type>
* <type>[TransRangeColour](TransRangeColour.md)</type>
* <type>[TransRangeDesat](TransRangeDesat.md)</type>
* <type>[TransRangeBlend](TransRangeBlend.md)</type>
* <type>[TransRangeTint](TransRangeTint.md)</type>
* <type>[TransRangeSpecial](TransRangeSpecial.md)</type>

## Constants

| Name | Value |
|:-----|:------|
`TYPE_PALETTE` | 0
`TYPE_COLOUR` | 1
`TYPE_DESAT` | 2
`TYPE_BLEND` | 3
`TYPE_TINT` | 4
`TYPE_SPECIAL` | 5

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">type</prop> | <type>integer</type> | The type of translation for this range (see `TYPE_` constants)
<prop class="rw">rangeStart</prop> | <type>integer</type> | The first palette index of the range (`0` - `255`)
<prop class="rw">rangeEnd</prop> | <type>integer</type> | The last palette index of the range (`0` - `255`)

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Translation.AddRange](Translation.md#addrange)</code>

## Functions

### Overview

#### General

<fdef>[AsText](#astext)() -> <type>string</type></fdef>

#### Derived Type Casts

<fdef>[AsPaletteRange](#aspaletterange)() -> <type>[TransRangePalette](TransRangePalette.md)</type></fdef>
<fdef>[AsColourRange](#ascolourrange)() -> <type>[TransRangeColour](TransRangeColour.md)</type></fdef>
<fdef>[AsDesatRange](#asdesatrange)() -> <type>[TransRangeDesat](TransRangeDesat.md)</type></fdef>
<fdef>[AsBlendRange](#asblendrange)() -> <type>[TransRangeBlend](TransRangeBlend.md)</type></fdef>
<fdef>[AsTintRange](#astintrange)() -> <type>[TransRangeTint](TransRangeTint.md)</type></fdef>
<fdef>[AsSpecialRange](#asspecialrange)() -> <type>[TransRangeSpecial](TransRangeSpecial.md)</type></fdef>

---
### AsText

Gets the text definition for this translation range (in ZDoom format).

<listhead>Returns</listhead>

* <type>string</type>: A text representation of the translation range

---
### AsPaletteRange

Casts to a <type>[TransRangePalette](TransRangePalette.md)</type>, if it is one.

<listhead>Returns</listhead>

* <type>[TransRangePalette](TransRangePalette.md)</type>: The range as a 'palette' type range, or `nil` if it is not that type

---
### AsColourRange

Casts to a <type>[TransRangeColour](TransRangeColour.md)</type>, if it is one.

<listhead>Returns</listhead>

* <type>[TransRangeColour](TransRangeColour.md)</type>: The range as a 'colour' type range, or `nil` if it is not that type

---
### AsDesatRange

Casts to a <type>[TransRangeDesat](TransRangeDesat.md)</type>, if it is one.

<listhead>Returns</listhead>

* <type>[TransRangeDesat](TransRangeDesat.md)</type>: The range as a 'desat' type range, or `nil` if it is not that type

---
### AsBlendRange

Casts to a <type>[TransRangeBlend](TransRangeBlend.md)</type>, if it is one.

<listhead>Returns</listhead>

* <type>[TransRangeBlend](TransRangeBlend.md)</type>: The range as a 'blend' type range, or `nil` if it is not that type

---
### AsTintRange

Casts to a <type>[TransRangeTint](TransRangeTint.md)</type>, if it is one.

<listhead>Returns</listhead>

* <type>[TransRangeTint](TransRangeTint.md)</type>: The range as a 'tint' type range, or `nil` if it is not that type

---
### AsSpecialRange

Casts to a <type>[TransRangeSpecial](TransRangeSpecial.md)</type>, if it is one.

<listhead>Returns</listhead>

* <type>[TransRangeSpecial](TransRangeSpecial.md)</type>: The range as a 'special' type range, or `nil` if it is not that type

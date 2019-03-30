<article-head>TransRange</article-head>

A single 'part' of a custom <type>[Translation](Translation.md)</type>, which can be made up of multiple (each one corresponding to a range in the palette).

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
<prop class="ro">type</prop> | <type>number</type> | The type of translation for this range (see `TYPE_` constants)
<prop class="rw">rangeStart</prop> | <type>number</type> | The first palette index of the range (`0` - `255`)
<prop class="rw">rangeEnd</prop> | <type>number</type> | The last palette index of the range (`0` - `255`)

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[Translation.AddRange](Translation.md#addrange)</code>

## Functions

### AsText

<fdef>function <type>TransRange</type>.<func>AsText</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>string</type>: A text representation of the translation range

---
### AsPaletteRange

<fdef>function <type>TransRange</type>.<func>AsPaletteRange</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[TransRangePalette](TransRangePalette.md)</type>: The range as a 'palette' type range, or `nil` if it is not that type

---
### AsColourRange

<fdef>function <type>TransRange</type>.<func>AsColourRange</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[TransRangeColour](TransRangeColour.md)</type>: The range as a 'colour' type range, or `nil` if it is not that type

---
### AsDesatRange

<fdef>function <type>TransRange</type>.<func>AsDesatRange</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[TransRangeDesat](TransRangeDesat.md)</type>: The range as a 'desat' type range, or `nil` if it is not that type

---
### AsBlendRange

<fdef>function <type>TransRange</type>.<func>AsBlendRange</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[TransRangeBlend](TransRangeBlend.md)</type>: The range as a 'blend' type range, or `nil` if it is not that type

---
### AsTintRange

<fdef>function <type>TransRange</type>.<func>AsTintRange</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[TransRangeTint](TransRangeTint.md)</type>: The range as a 'tint' type range, or `nil` if it is not that type

---
### AsSpecialRange

<fdef>function <type>TransRange</type>.<func>AsSpecialRange</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[TransRangeSpecial](TransRangeSpecial.md)</type>: The range as a 'special' type range, or `nil` if it is not that type

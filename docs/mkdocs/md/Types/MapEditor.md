
Map editing context for the currently open map editor in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">editMode</prop>        | <type>number</type> | The current edit mode (see `MODE_` constants below)
<prop class="ro">sectorEditMode</prop>  | <type>number</type> | The current sector edit mode (see `SECTORMODE_` constants below)
<prop class="ro">gridSize</prop>        | <type>number</type> | The current grid size
<prop class="ro">map</prop>             | <type>[Map](Map.md)</type> | The map associated with this editor

## Constants

| Name | Value |
|:-----|:-----:|
`MODE_VERTICES` | 0
`MODE_LINES` | 1
`MODE_SECTORS` | 2
`MODE_THINGS` | 3
`MODE_VISUAL` | 4
`SECTORMODE_BOTH` | 0
`SECTORMODE_FLOOR` | 1
`SECTORMODE_CEILING` | 2

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions - General

### `SetEditMode`

<listhead>Parameters</listhead>

* <type>number</type> <arg>mode</arg>: The edit mode to switch to (see `MODE_` constants)
* `[`<type>number</type> <arg>sectorMode</arg> : `SECTORMODE_BOTH]`: The sector edit mode to switch to (see `SECTORMODE_` constants)

Sets the edit mode to the given <arg>mode</arg>. If the mode is being set to `MODE_SECTORS`, the <arg>sectorMode</arg> parameter can be given to specify the sector edit mode.

## Functions - Selection

### `ClearSelection`

Deselects all currently selected items

---
### `Select`

<listhead>Parameters</listhead>

* <type>[MapObject](MapObject.md)</type> <arg>object</arg>: The <type>[MapObject](MapObject.md)</type> to (de)select
* `[`<type>boolean</type> <arg>select</arg> : `true]`: Whether to select or deselect the object

Selects or deselects the given <type>[MapObject](MapObject.md)</type> (or derived type), depending on <arg>select</arg>.

---
### `SelectedLines`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>tryHighlight</arg> : `false]`: Whether to get the current highlight if nothing is selected

**Returns** <type>[MapLine](MapLine.md)\[\]</type>

Returns an array of all currently selected lines. If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted line is returned in the array.

---
### `SelectedSectors`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>tryHighlight</arg> : `false]`: Whether to get the current highlight if nothing is selected

**Returns** <type>[MapSector](MapSector.md)\[\]</type>

Returns an array of all currently selected sectors. If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted sector is returned in the array.

---
### `SelectedThings`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>tryHighlight</arg> : `false]`: Whether to get the current highlight if nothing is selected

**Returns** <type>[MapThing](MapThing.md)\[\]</type>

Returns an array of all currently selected things. If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted thing is returned in the array.

---
### `SelectedVertices`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>tryHighlight</arg> : `false]`: Whether to get the current highlight if nothing is selected

**Returns** <type>[MapVertex](MapVertex.md)\[\]</type>

Returns an array of all currently selected vertices. If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted vertex is returned in the array.

<subhead>Type</subhead>
<header>MapEditor</header>

Map editing context for the currently open map editor in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">editMode</prop>        | <type>integer</type> | The current edit mode (see `MODE_` constants below)
<prop class="ro">sectorEditMode</prop>  | <type>integer</type> | The current sector edit mode (see `SECTORMODE_` constants below)
<prop class="ro">gridSize</prop>        | <type>integer</type> | The current grid size
<prop class="ro">map</prop>             | <type>[Map](Map.md)</type> | The map associated with this editor

## Constants

| Name | Value |
|:-----|:------|
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

**See:**

* <code>[App.MapEditor](../../Namespaces/App.md#mapeditor)</code>

## Functions

### Overview

#### General

<fdef>[SetEditMode](#seteditmode)(<arg>mode</arg>, <arg>[sectorMode]</arg>)</fdef>

#### Selection

<fdef>[ClearSelection](#clearselection)()</fdef>
<fdef>[Select](#select)(<arg>object</arg>, <arg>[add]</arg>)</fdef>
<fdef>[SelectedLines](#selectedlines)(<arg>[tryHighlight]</arg>) -> <type>[MapLine](MapLine.md)\[\]</type></fdef>
<fdef>[SelectedSectors](#selectedsectors)(<arg>[tryHighlight]</arg>) -> <type>[MapSector](MapSector.md)\[\]</type></fdef>
<fdef>[SelectedThings](#selectedthings)(<arg>[tryHighlight]</arg>) -> <type>[MapThing](MapThing.md)\[\]</type></fdef>
<fdef>[SelectedVertices](#selectedvertices)(<arg>[tryHighlight]</arg>) -> <type>[MapVertex](MapVertex.md)\[\]</type></fdef>

---
### SetEditMode

Sets the edit mode to the given <arg>mode</arg>. If the mode is being set to `MODE_SECTORS`, the <arg>sectorMode</arg> parameter can be given to specify the sector edit mode.

#### Parameters

* <arg>mode</arg> (<type>integer</type>): The edit mode to switch to (see `MODE_` constants)
* <arg>[sectorMode]</arg> (<type>integer</type>): The sector edit mode to switch to (see `SECTORMODE_` constants). Default is `SECTORMODE_BOTH`

---
### ClearSelection

Deselects all currently selected items

---
### Select

Selects or deselects the given <type>[MapObject](MapObject.md)</type> (or derived type), depending on <arg>add</arg>.

#### Parameters

* <arg>object</arg> (<type>[MapObject](MapObject.md)</type>): The <type>[MapObject](MapObject.md)</type> to (de)select
* <arg>[add]</arg> (<type>boolean</type>): Whether to add (`true`) or remove (`false`) from the current selection. Default is `true`

---
### SelectedLines

#### Parameters

* <arg>[tryHighlight]</arg> (<type>boolean</type>): Whether to get the current highlight if nothing is selected. Default is `false`

#### Returns

* <type>[MapLine](MapLine.md)\[\]</type>: An array of all currently selected lines

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted line is returned in the array.

---
### SelectedSectors

#### Parameters

* <arg>[tryHighlight]</arg> (<type>boolean</type>): Whether to get the current highlight if nothing is selected. Default is `false`

#### Returns

* <type>[MapSector](MapSector.md)\[\]</type>: An array of all currently selected sectors

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted sector is returned in the array.

---
### SelectedThings

#### Parameters

* <arg>[tryHighlight]</arg> (<type>boolean</type>): Whether to get the current highlight if nothing is selected. Default is `false`

#### Returns

* <type>[MapThing](MapThing.md)\[\]</type>: An array of all currently selected things

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted thing is returned in the array.

---
### SelectedVertices

#### Parameters

* <arg>[tryHighlight]</arg> (<type>boolean</type>): Whether to get the current highlight if nothing is selected. Default is `false`

#### Returns

* <type>[MapVertex](MapVertex.md)\[\]</type>: An array of all currently selected vertices

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted vertex is returned in the array.

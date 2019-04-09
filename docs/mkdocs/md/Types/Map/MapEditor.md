<article-head>MapEditor</article-head>

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

<listhead>See:</listhead>

* <code>[App.MapEditor](../../Namespaces/App.md#mapeditor)</code>

## Functions - General

### SetEditMode

<fdef>function <type>MapEditor</type>.<func>SetEditMode</func>(<arg>*self*</arg>, <arg>mode</arg>, <arg>sectorMode</arg>)</fdef>

Sets the edit mode to the given <arg>mode</arg>. If the mode is being set to `MODE_SECTORS`, the <arg>sectorMode</arg> parameter can be given to specify the sector edit mode.

<listhead>Parameters</listhead>

* <arg>mode</arg> (<type>number</type>): The edit mode to switch to (see `MODE_` constants)
* <arg>[sectorMode]</arg> (<type>number</type>, default `SECTORMODE_BOTH`): The sector edit mode to switch to (see `SECTORMODE_` constants)

## Functions - Selection

### ClearSelection

<fdef>function <type>MapEditor</type>.<func>ClearSelection</func>(<arg>*self*</arg>)</fdef>

Deselects all currently selected items

---
### Select

<fdef>function <type>MapEditor</type>.<func>Select</func>(<arg>*self*</arg>, <arg>object</arg>, <arg>add</arg>)</fdef>

Selects or deselects the given <type>[MapObject](MapObject.md)</type> (or derived type), depending on <arg>add</arg>.

<listhead>Parameters</listhead>

* <arg>object</arg> (<type>[MapObject](MapObject.md)</type>): The <type>[MapObject](MapObject.md)</type> to (de)select
* <arg>[add]</arg> (<type>boolean</type>, default `true`): Whether to add (`true`) or remove (`false`) from the current selection

---
### SelectedLines

<fdef>function <type>MapEditor</type>.<func>SelectedLines</func>(<arg>*self*</arg>, <arg>tryHighlight</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>[tryHighlight]</arg> (<type>boolean</type>, default `false`): Whether to get the current highlight if nothing is selected

<listhead>Returns</listhead>

* <type>[MapLine](MapLine.md)\[\]</type>: An array of all currently selected lines

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted line is returned in the array.

---
### SelectedSectors

<fdef>function <type>MapEditor</type>.<func>SelectedSectors</func>(<arg>*self*</arg>, <arg>tryHighlight</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>[tryHighlight]</arg> (<type>boolean</type>, default `false`): Whether to get the current highlight if nothing is selected

<listhead>Returns</listhead>

* <type>[MapSector](MapSector.md)\[\]</type>: An array of all currently selected sectors

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted sector is returned in the array.

---
### SelectedThings

<fdef>function <type>MapEditor</type>.<func>SelectedThings</func>(<arg>*self*</arg>, <arg>tryHighlight</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>[tryHighlight]</arg> (<type>boolean</type>, default `false`): Whether to get the current highlight if nothing is selected

<listhead>Returns</listhead>

* <type>[MapThing](MapThing.md)\[\]</type>: An array of all currently selected things

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted thing is returned in the array.

---
### SelectedVertices

<fdef>function <type>MapEditor</type>.<func>SelectedVertices</func>(<arg>*self*</arg>, <arg>tryHighlight</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>[tryHighlight]</arg> (<type>boolean</type>, default `false`): Whether to get the current highlight if nothing is selected

<listhead>Returns</listhead>

* <type>[MapVertex](MapVertex.md)\[\]</type>: An array of all currently selected vertices

#### Notes

If nothing is selected and <arg>tryHighlight</arg> is `true`, the currently highlighted vertex is returned in the array.

<subhead>Type</subhead>
<header>Map</header>

Contains information and structures representing a Doom map.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">name</prop>          | <type>string</type> | The name of the map (eg. `MAP01`)
<prop class="ro">udmfNamespace</prop> | <type>string</type> | The UDMF namespace of the map. Will be blank if not in UDMF format
<prop class="ro">vertices</prop>      | <type>[MapVertex](MapVertex.md)\[\]</type> | An array of all vertices in the map
<prop class="ro">linedefs</prop>      | <type>[MapLine](MapLine.md)\[\]</type> | An array of all lines in the map
<prop class="ro">sidedefs</prop>      | <type>[MapSide](MapSide.md)\[\]</type> | An array of all sides in the map
<prop class="ro">sectors</prop>       | <type>[MapSector](MapSector.md)\[\]</type> | An array of all sectors in the map
<prop class="ro">things</prop>        | <type>[MapThing](MapThing.md)\[\]</type> | An array of all things in the map

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

### Overview

#### Creation

<fdef>[CreateVertex](#createvertex)(<arg>position</arg>, <arg>[splitDist]</arg>) -> <type>[MapVertex](MapVertex.md)</type></fdef>
<fdef>[CreateLine](#createline)(<arg>start</arg>, <arg>end</arg>, <arg>[splitDist]</arg>) -> <type>[MapLine](MapLine.md)</type></fdef>
<fdef>[CreateLine](#createline)(<arg>vertex1</arg>, <arg>vertex2</arg>, <arg>[force]</arg>) -> <type>[MapLine](MapLine.md)</type></fdef>
<fdef>[CreateThing](#creatething)(<arg>position</arg>, <arg>[type]</arg>) -> <type>[MapThing](MapThing.md)</type></fdef>
<fdef>[CreateSector](#createsector)() -> <type>[MapSector](MapSector.md)</type></fdef>
<fdef>[CreateSide](#createside)(<arg>sector</arg>) -> <type>[MapSide](MapSide.md)</type></fdef>

#### Removal

<fdef>[RemoveVertex](#removevertex)(<arg>vertex</arg>, <arg>[mergeLines]</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveVertex](#removevertex)(<arg>index</arg>, <arg>[mergeLines]</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveLine](#removeline)(<arg>line</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveLine](#removeline)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveSide](#removeside)(<arg>side</arg>, <arg>[removeFromLine]</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveSide](#removeside)(<arg>index</arg>, <arg>[removeFromLine]</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveSector](#removesector)(<arg>sector</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveSector](#removesector)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveThing](#removething)(<arg>thing</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveThing](#removething)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[RemoveDetachedVertices](#removedetachedvertices)() -> <type>integer</type></fdef>

#### Geometry Editing

<fdef>[MergeVertices](#mergevertices)(<arg>vertex1</arg>, <arg>vertex2</arg>)</fdef>
<fdef>[MergeVerticesPoint](#mergeverticespoint)(<arg>position</arg>) -> <type>[MapVertex](MapVertex.md)</type></fdef>
<fdef>[SplitLine](#splitline)(<arg>line</arg>, <arg>vertex</arg>) -> <type>[MapLine](MapLine.md)</type></fdef>
<fdef>[SplitLinesAt](#splitlinesat)(<arg>vertex</arg>, <arg>[splitDist]</arg>)</fdef>
<fdef>[SetLineSector](#setlinesector)(<arg>lineIndex</arg>, <arg>sectorIndex</arg>, <arg>[front]</arg>) -> <type>boolean</type></fdef>
<fdef>[MergeLine](#mergeline)(<arg>index</arg>) -> <type>integer</type></fdef>
<fdef>[CorrectLineSectors](#correctlinesectors)(<arg>line</arg>) -> <type>boolean</type></fdef>
<fdef>[SetLineSide](#setlineside)(<arg>line</arg>, <arg>side</arg>, <arg>front</arg>)</fdef>
<fdef>[CorrectSectors](#correctsectors)(<arg>lines</arg>, <arg>[existingOnly]</arg>)</fdef>

---
### CreateVertex

Creates a vertex at <arg>position</arg>. If <arg>splitDist</arg> is specified, nearby lines within that distance will be split by the new vertex.

---
### CreateLine

Creates a line from two points or from two existing vertices. When using points, missing vertices are created automatically. If <arg>force</arg> is `false` or omitted, an existing line between the same vertices may be returned instead.

---
### CreateThing

Creates a thing at <arg>position</arg>. If <arg>type</arg> is omitted, type `1` is used.

---
### CreateSector

Creates an empty sector.

---
### CreateSide

Creates a side attached to <arg>sector</arg>.

---
### RemoveVertex

Removes a vertex object or vertex index. If <arg>mergeLines</arg> is `true`, connected lines may be merged where possible.

---
### RemoveLine

Removes a line object or line index.

---
### RemoveSide

Removes a side object or side index. If <arg>removeFromLine</arg> is omitted, the side is also removed from its parent line.

---
### RemoveSector

Removes a sector object or sector index.

---
### RemoveThing

Removes a thing object or thing index.

---
### RemoveDetachedVertices

Removes all vertices not connected to any lines.

#### Returns

* <type>integer</type>: The number of vertices removed

---
### MergeVertices

Merges the vertex at <arg>vertex2</arg> into the vertex at <arg>vertex1</arg>, then removes <arg>vertex2</arg>.

---
### MergeVerticesPoint

Merges all vertices near <arg>position</arg>.

#### Returns

* <type>[MapVertex](MapVertex.md)</type>: The remaining merged vertex

---
### SplitLine

Splits <arg>line</arg> at <arg>vertex</arg>.

#### Returns

* <type>[MapLine](MapLine.md)</type>: The new line created by the split

---
### SplitLinesAt

Splits all lines close to <arg>vertex</arg>. If <arg>splitDist</arg> is omitted, `0` is used.

---
### SetLineSector

Sets a line's front or back side to reference a sector, creating a side if needed.

#### Parameters

* <arg>lineIndex</arg> (<type>integer</type>): The line index
* <arg>sectorIndex</arg> (<type>integer</type>): The sector index
* <arg>[front]</arg> (<type>boolean</type>): If `true` or omitted, set the front side. If `false`, set the back side

#### Returns

* <type>boolean</type>: `true` if a new side was created

---
### MergeLine

Removes any lines overlapping the line at <arg>index</arg>, and fixes sector references afterwards.

#### Returns

* <type>integer</type>: The number of lines removed

---
### CorrectLineSectors

Attempts to set <arg>line</arg>'s side sector references to the correct sectors.

---
### SetLineSide

Sets <arg>line</arg>'s front or back side to <arg>side</arg>. If <arg>side</arg> already belongs to another line, it is copied.

---
### CorrectSectors

Builds or corrects sectors from the supplied <arg>lines</arg>. If <arg>existingOnly</arg> is `true`, only existing sides are used when tracing sectors.

**See:**

* <code>[MapEditor.map](MapEditor.md#properties)</code>

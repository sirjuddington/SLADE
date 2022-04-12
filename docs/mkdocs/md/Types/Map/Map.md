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

**See:**

* <code>[MapEditor.map](MapEditor.md#properties)</code>

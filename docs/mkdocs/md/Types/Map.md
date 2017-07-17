
Contains information and structures representing a Doom map.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>name</prop>          | <type>string</type> | The name of the map (eg. `MAP01`)
<prop>udmfNamespace</prop> | <type>string</type> | The UDMF namespace of the map. Will be blank if not in UDMF format
<prop>vertices</prop>      | <type>[MapVertex](MapVertex.md)\[\]</type> | An array of all vertices in the map
<prop>linedefs</prop>      | <type>[MapLine](MapLine.md)\[\]</type> | An array of all lines in the map
<prop>sidedefs</prop>      | <type>[MapSide](MapSide.md)\[\]</type> | An array of all sides in the map
<prop>sectors</prop>       | <type>[MapSector](MapSector.md)\[\]</type> | An array of all sectors in the map
<prop>things</prop>        | <type>[MapThing](MapThing.md)\[\]</type> | An array of all things in the map

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

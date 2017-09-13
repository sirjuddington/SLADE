A geometrical [Plane](https://en.wikipedia.org/wiki/Plane_(geometry)), represented in general form (<prop>a</prop>`x+`<prop>b</prop>`y+`<prop>c</prop>`z+`<prop>d </prop>`= 0`).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">a</prop> | <type>number</type> | Normal X component
<prop class="rw">b</prop> | <type>number</type> | Normal Y component
<prop class="rw">c</prop> | <type>number</type> | Normal Z component
<prop class="rw">d</prop> | <type>number</type> | Distance component

## Constructors

`new` `(` `)`  
Creates a new plane with all components set to `0`.

`new` `(` <type>number</type> <arg>a</arg>, <type>number</type> <arg>b</arg>, <type>number</type> <arg>c</arg>, <type>number</type> <arg>d</arg> `)`  
Creates a new plane with the given <arg>a</arg>, <arg>b</arg>, <arg>c</arg> and <arg>d</arg> components.

## Functions

### `heightAt`

<listhead>Parameters</listhead>

* <type>[Point](Point.md)</type> <arg>position</arg>: The 2D position to find the plane height at

**Returns** <type>number</type>

Returns the height (`z`) on the plane at <arg>position</arg>

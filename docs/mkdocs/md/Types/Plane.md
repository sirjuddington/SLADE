<subhead>Type</subhead>
<header>Plane</header>

A geometrical [Plane](https://en.wikipedia.org/wiki/Plane_(geometry)), represented in general form (<prop>a</prop>`x+`<prop>b</prop>`y+`<prop>c</prop>`z+`<prop>d </prop>`= 0`).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">a</prop> | <type>float</type> | Normal X component
<prop class="rw">b</prop> | <type>float</type> | Normal Y component
<prop class="rw">c</prop> | <type>float</type> | Normal Z component
<prop class="rw">d</prop> | <type>float</type> | Distance component

## Constructors

<code><type>Plane</type>.<func>new</func>()</code>

Creates a new plane with all components set to `0`.

---

<code><type>Plane</type>.<func>new</func>(<arg>a</arg>, <arg>b</arg>, <arg>c</arg>, <arg>d</arg>)</code>

Creates a new plane with the given <arg>a</arg>, <arg>b</arg>, <arg>c</arg> and <arg>d</arg> components.

#### Parameters

* <arg>a</arg> (<type>float</type>): The <prop>a</prop> component (normal X)
* <arg>b</arg> (<type>float</type>): The <prop>b</prop> component (normal Y)
* <arg>c</arg> (<type>float</type>): The <prop>c</prop> component (normal Z)
* <arg>d</arg> (<type>float</type>): The <prop>d</prop> component (distance)

## Functions

### Overview

<fdef>[HeightAt](#heightat)(<arg>position</arg>) -> <type>float</type></fdef>

---
### HeightAt

Calculates the height (`z`) at <arg>position</arg> on the plane.

#### Parameters

* <arg>position</arg> (<type>[Point](Point.md)</type>): The 2D position to find the plane height at

#### Returns

* <type>float</type>: The height (`z`) on the plane at <arg>position</arg>

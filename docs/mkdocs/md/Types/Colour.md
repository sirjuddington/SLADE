Represents an RGBA colour. Note that colour each component is 8-bit (ie. must be between `0` and `255`).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>r</prop> | <type>number</type> | Red component
<prop>g</prop> | <type>number</type> | Green component
<prop>b</prop> | <type>number</type> | Blue component
<prop>a</prop> | <type>number</type> | Alpha component (transparency)

## Constructors

`new` `(` `)`  
Creates a new colour with all components set to `0`.

`new` `(` <type>number</type> <arg>r</arg>, <type>number</type> <arg>g</arg>, <type>number</type> <arg>b</arg> `)`  
Creates a new colour with the given <arg>r</arg>, <arg>g</arg> and <arg>b</arg> components. The <prop>a</prop> component is set to `255`.

`new` `(` <type>number</type> <arg>r</arg>, <type>number</type> <arg>g</arg>, <type>number</type> <arg>b</arg>, <type>number</type> <arg>a</arg> `)`  
Creates a new colour with the given <arg>r</arg>, <arg>g</arg>, <arg>b</arg> and <arg>a</arg> components.

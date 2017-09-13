
Documentation for SLADE Lua scripting types and functions.

!!! Warning
    This is all very WIP, expect changes before 3.1.2 is released

## Notes

* Type properties are shown with a prefixed symbol indicating the following:
    * Red &#9675;: Read-only, can not be modified
    * Green &#9679;: Can be modified
* State is not presereved after running a script - eg. a global variable defined in a script will not be available next time a script is run

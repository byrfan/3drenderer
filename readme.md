A Model render that takes in various file formats (to be decided)

    `interpret_obj.c` -> will contain structure for serialising the file
        ? global structure system, at least a structure system derived 
        by the obj format. this leads to possible conversion of 3d file
        formats.

TODO:
    - Add support for other file formats
    - fix all the different syntactic naming of functions
        -> function names should be:
            "does-this_thing"
        -> variables should be:
            "this"
        -> macros or definitions should be:
            "ThisThing"
    - Try to contextify global variables and leave them in scope.
        -> depth_buffer and pixels can be put in one class (?) and left in the
        scope of the Run_Window() function 

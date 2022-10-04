Lkey
======
Lkey is a MIDI keyboard that can play chords with a single keypress. 

Requirements
-------------
* GTK4 
* JACK, the Jack Audio Connection Kit.

Installation
-----------
In the source directory, do:

    $ meson build
    $ cd build
    $ meson compile
    $ sudo meson install

How to Use
----------
Lkey's keyboard starts at the 'Z' key on your computer keyboard and ends at the
'/' key. These correspond to musical notes C through E.

Switch chords: 0-9, arrow keys, or left click.
Change octave: '+' and '-'.
Add a new chord: Right click on a chord label, play a chord, then press
'Enter'.
Invert the active chord: '[' and ']'

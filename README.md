# AstroED - ASTROphotos EDitor
## inspired by siril and many others windows software

This will be program to stuck astropictures for Linux, user friendly.
For now it will be based on siril (http://free-astro.org/index.php/Siril).

Currently program is at very early developing stage.

Implemented:
* video parser - if program meets video files it automatically lists all frames inside like it is folder with files (virtual filesystem in fact)
* lua editor - made LUA text editor with autocomplete, highlights, indents.
* zoomable preview of original image
* EXIF reading from JPEGS
* smooth scroll/preview, fast scroll with HDD (magnetic) + i7 cpu + 8Gb ram of 33000 jpeg pictures 20MPx each (half of them are frames from videos, so less resolution, but encoder used, also smooth in backward scroll)

TODO / Ideas:
* lua project - okey, initial idea does not look cool. So what I think. It will be "project" savable, but file-format of this will be lua, nothing special, just parsing globals using separated VM.
* lua_code - this part will have own VM and API like getGUIFilesListed(), also it will have access to math & algos. I think that will work like assembler. I.e. it will be "accumulator" image which is
1 of operands and keeps result. That can be shown as preview additionally (possibly, in separated window).


fig
===

fig is an image and animation library written in C.

Supports:

* Graphics Interchange Format (.gif) - Reading and writing.

Using
-----

* **Single-header version**: Just drop the fig.h file that is included in `single_header/` folder into your project and start using the library.
* **Visual Studio** (Windows): Make sure a recent version of Visual Studio is installed. The solution for Visual Studio 2015 is in the `vc/` folder. Open and Build Solution, or add the vcproj into an already existing solution and adjust as necessary.
* **Makefile** (Mac / Linux / Windows (MinGW + GnuWin32) / Cygwin / etc): Make sure GNU Make, and either GCC or Clang is installed. Run `make` in the base directory of the repository.

License
-------

The source code for fig is released under the MIT license:

    Copyright © 2016 Andrew G. Crowell

    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal in
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

The examples have their own licenses:

* `FullColourGIF.gif` from http://commons.wikimedia.org/wiki/File:FullColourGIF.gif is licensed under a [Creative Commons Attribution-ShareAlike 3.0 Unported License][1] by GDallimore.

[1]: http://creativecommons.org/licenses/by-sa/3.0/

References
----------

* Matthew Flickinger. [Project: What's In A GIF - Bit by Byte][2].
* W3C. [Cover Sheet for the GIF89a Specification][3].
* Steve Blackstock. [LZW and GIF explained][4].
* Bob Montgomery. [LZW compression used to encode/decode a GIF file][5].
* stb. [stb_image.h Source Code][6].
* ImageMagick v6 Documentation. [ImageMagick v6 Examples -- Animation Basics][7].

[2]: http://www.matthewflickinger.com/lab/whatsinagif/bits_and_bytes.asp
[3]: http://www.w3.org/Graphics/GIF/spec-gif89a.txt
[4]: http://gingko.homeip.net/docs/file_formats/lzwgif.html#ste
[5]: http://gingko.homeip.net/docs/file_formats/lzwgif.html#bob
[6]: https://github.com/nothings/stb/blob/master/stb_image.h
[7]: http://www.imagemagick.org/Usage/anim_basics/

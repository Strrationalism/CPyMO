
OVERVIEW
--------
GNU Unifont is an official GNU package.  It is a dual-width
bitmap font, 16 pixels tall by 8 or 16 pixels wide.  Experimental
use of 24 and 32 pixel wide glyphs is also supported by some
utility programs; consult the man pages and texinfo documentation.
Unifont is designed to provide coverage for all of Unicode Plane 0,
the Basic Multilingual Plane (BMP).

GNU Unifont has a glyph for each visible code point in the
Unicode Basic Multilingual Plane (Plane 0) and some glyphs
in the Supplemental Multilingual Plane (Plane 1).  This version
also includes many glyphs in Michael Everson's ConScript
Unicode Registry (CSUR).

Unifont only provides a single glyph for each character, making it
impossible to handle any language properly that needs context-dependent
character shaping. It is supplied in the form of a hex file, with
a converter to convert it to BDF. See http://czyborra.com/unifont/
or http://unifoundry.com/unifont.html for more information.  The
BDF font is converted to PCF, and the hex file is converted to a
TrueType font.

This is the unifoundry.com collection of utilities for GNU Unifont,
assembled by Paul Hardy with the encouragement of the font's creator,
Roman Czyborra.  This archive contains the following directories
and files:

     ChangeLog  Log of changes made to each GNU release
     COPYING    Full text of GPL version 2
     doc        Documentation in Texinfo format
     doxygen    Documentation in doxygen format (html/index.html and
                   unifont-doxy.pdf)
     font       The font source file with scripts for building
     hangul     Standalone font sources to build hangul-syllables.hex
     INSTALL    Instructions for font and software installation
     Makefile   The "make" file
     man        Unix man pages
     NEWS       Summary of what's new with each GNU release
     README     This file
     src        Source programs, in Perl and C

The "font/precompiled" directory contains prebuilt font-related files:

     coverage.txt                     Percentage coverage of Plane 0 scripts

     unifont-<version>.hex            Hex string source of glyphs to build
                                      Unifont
     unifont-<version>.bdf.gz         BDF version of Unifont
     unifont-<version>.pcf.gz         PCF version of Unifont
     unifont-<version>.ttf            TrueType version of Unifont

     unifont_jp-<version>.*           Version of Unifont with Japanese glyphs
                                      mapped from Plane 1 and Plane 2 of the
                                      JIS X 0213 standard to Unicode.  The
                                      TrueType version includes those Unicode
                                      Plane 2 glyphs that appear in JIS X 0213.

     unifont_sample-<version>.hex     Hex string source of all Plane 0 glyphs
                                      (except those for U+FFFE and U+FFFF),
                                      including nonprinting and PUA glyphs,
                                      with combining circles
     unifont_sample-<version>.bdf.gz  BDF font version of the above .hex file
     unifont_sample-<version>.ttf     SBIT font version of the above .hex file

     unifont_csur-<version>.*         Fonts containing Plane 0 Unifont glyphs
                                      plus glyphs for Michael Everson's
                                      ConScript Unicode Registry (CSUR) for
                                      the Plane 0 Private Use Area

     unifont_upper-<version>.*        Fonts containing glyphs from Unicode
                                      Plane 1 through Plane 14, inclusive

     unifont_upper_csur-<version>.*   Fonts containing glyphs from Unifont
                                      Upper plus glyphs from Michael Everson's
                                      ConScript Unicode Registry (CSUR) that
                                      are in the Private Use Area in Plane 15

     Unifont-APL8x16-<version>.psf.gz Monospace console font for using APL
                                      (GNU APL, etc.) in single-user mode.

     unifont-<version>.bmp            The entire Plane 0 Unifont font with
                                      combining circles, built from the files
                                      font/plane00/*.hex, showing combining
                                      circles

The directory that was originally named "font/hexsrc" has been renamed
to "font/plane00" now that Unifont supports glyphs beyond Plane 0.  Higher
plane glyphs appear in "font/plane01" through "font/plane0F".  Currently
there is no "font/plane10" directory (the highest Unicode plane is Plane 17,
or 0x10).

This release incorporates all glyph errata issued by The Unicode Consortium
from Unicode 1.0 errata to the latest.


BUILDING
--------
See the "INSTALL" file in this directory for building instructions.


PRIVATE USE AREA (PUA)
----------------------
The "pua.hex" file contains a four-digit hexadecimal representation of
each code point, rendered as white on black.  The new program "hexgen.c"
generated these glyphs.  A four-digit hexadecimal code point is suggested
as one possible rendering of PUA glyphs in The Unicode Standard, Version
5.0, Section 5.3.  Another possible rendering suggested in that same
section is a pencil glyph.  A pencil glyph was used originally in Unifont
Version 5.1.

The glyphs in "pua.hex" are not compiled into the final font.  To do
so, modify font/Makefile by adding "pua.hex" to the list of hex source
files.  Alternatively, someone could use their own pua.hex file for
various Private Use Area assignments.

To use these glyphs in the PUA, use the command

     make BUILDFONT=1 PUA=plane00/pua.hex

The glyphs in font/plane00/hangul/hangul-base.hex begin at 0xE000,
and so can also be used as PUA glyphs.  To use these glyphs in the
PUA, use the command

     make BUILDFONT=1 PUA=plane00/hangul/hangul-base.hex

The unifont_all-$(VERSION).hex will contain Unicode Plane 0 PUA
glyphs from Michael Everson's ConScript Unicode Registry (CSUR)
plus Rebecca Bettencourt's Under CSUR (UCSUR) additions.


src/ AUTHORS
------------
Roman Czyborra wrote all the Perl files in the src directory except
"hex2sfd", "hexkinya", "unifontchojung", "unifontksx", "unihex2png",
and "unipng2hex".

In the case of "johab2ucs2", Jungshik Shin wrote the orignial version;
he then gave it to Roman.  Paul Hardy made further changes to "johab2ucs2".

Roman originally named the "src/hexbraille" script as simply "braille".
Paul Hardy thought there was too great a chance of a name conflict with
other utilities, and so renamed it.

Luis Alejandro Gonzalez Miranda wrote the original "hex2sfd" Perl
script, as well as a "howto-build.sh" shell script that Paul Hardy
converted into "./font/ttfsrc/Makefile".

Paul Hardy wrote "unifontchojung" and "unifontksx" for extracting subsets
of Hangul glyphs, as an aid in creating a new Hangul Syllables block.

Andrew Miller wrote "hexkinya".  He also wrote "unihex2png" and "unipng2hex"
based upon Paul Hardy's "unihex2bmp" and "unibmp2hex" programs.  Last but not
least, he wrote the "unifont-viewer" Perl script to graphically view a Unifont
hex file dynamically.

David Corbett contributed the font/unipatch-hex.awk script, used
to replace glyphs in an existing Unifont .hex file with new glyphs.
This script is used to replace Chinese variation ideographs with
Japanese versions for the "unifont_jp*" font files.  He also wrote
src/hexrotate to simplify rotating some glyphs such as Mongolian,
which can be written vertically or horizontally.

Paul Hardy wrote all the C programs except for hex2otf.c, which
何志翔 (He Zhixiang) contributed.


Unifont AUTHORS
---------------
Roman Czyborra created the original GNU Unifont, including the
.hex format.  For greater detail, see the HISTORY section below.

David Starner aggregated many glyphs contributed by others and
incorporated these into pre-2004 Unifont releases.

Qianqian Fang began his Wen Quan Yi font in 2004, by which
time work on Unifont had stopped.  Most of the almost 30,000
CJK ideographs in Unifont versions 5.1 and later were taken
from Wen Quan Yi with permission of Qianqian Fang.  The glyphs
in "./font/plane00/wqy-cjk.hex" are for the most part Qianqian
Fang's Unibit and Wen Quan Yi glyphs.

Paul Hardy drew most of the newly-drawn glyphs added to the BMP
from the Unifont 5.1 release to the present release.  This includes
the 11,172 glyphs in the Hangul Syllables block, plus approximately
10,000 additional glyphs scattered throughout the BMP.

Andrew Miller drew the glyphs added to Unicode 6.3.0.

For full details of software changes after Unicode 6.3.0, higher
Unicode planes, and the Private Use Area glyphs, etc., see the
ChangeLog file.


LICENSE
-------
The source code for everything except the compiled fonts in this current
release is licensed as follows:

     License for this current distribution of program source
     files (i.e., everything except the fonts) is released under
     the terms of the GNU General Public License version 2,
     or (at your option) a later version.

GPL version 2 is contained in the "COPYING" file in the main source
directory for this package.  If your received this source without
a copy of GPL version 2, you can download a copy from GNU's website
at http://www.gnu.org/licenses/gpl-2.0.html.

The license for the compiled fonts is covered by the above GPL terms
with the GNU font embedding exception, as follows:

     As a special exception, if you create a document which uses this font,
     and embed this font or unaltered portions of this font into the document,
     this font does not by itself cause the resulting document to be covered
     by the GNU General Public License. This exception does not however
     invalidate any other reasons why the document might be covered by the
     GNU General Public License. If you modify this font, you may extend
     this exception to your version of the font, but you are not obligated
     to do so. If you do not wish to do so, delete this exception statement
     from your version. 

See "http://www.gnu.org/licenses/gpl-faq.html#FontException" for more details.


CHANGES IN VERSION 6.3
----------------------
Version 6.3 reflects all glyph changes and errata published in Unicode
6.3.0.  In preparation for releasing this version, Paul Hardy obtained
a hard copy of the errata published in Unicode Version 1.1, not yet
available on Unicode's website.  All previously published errata have
been incorporated.  This is a complete replacement for all previous
releases.

The following code points in previously published errata were examined
and found to be correct:

     Unicode 1.1: U+717F, U+773E, U+809C, U+8480, U+908E

Andrew Miller drew the 5 new additions to the Unicode 6.3.0 Basic
Multilingual Plane in the initial Unifont 6.3 release.

The latest Unifont 6.3 release includes these glyph changes by Paul Hardy:

   - Armenian -- several glyphs were redrawn based upon feedback from
     native speakers (U+0530..U+058F).

   - CJK Radicals Supplement -- several glyphs were redrawn to better match
     their representations in The Unicode Standard code charts:
     U+2E9F, U+2EA9, U+2EAC, U+2EAE, U+2EC0, U+2EDE, U+2EE7, and U+2EED.

   - Capricorn sign (U+2651) -- this was redrawn to an alternate form that
     better fit in an 8 by 16 pixel grid.

   - Dashes -- changed to distguish better between different dash types
     (a two horizontal pixel difference is the minimum to easily distinguish
     a difference between two glyphs):
     * Hyphen (U+002D) and Soft Hyphen (U+00AD) are now 4 pixels wide
     * En Dash (U+2012) is now 6 pixels wide
     * Em Dash (U+2013) is now 8 pixels wide

   - Control Pictures:
     * Centered text for C1 Controls U+0089 ("HTJ"), U+0095 ("MW"),
       and U+009E ("PM")
     * Copied glyphs from U+0000..U+001F to U+2400..u+241F and erased
       surrounding borders; earlier, some glyphs in U+0000..U+001F had
       their text re-centered so this carries that change forward

   - Arrows -- General Re-alignment
     * Aligned most single vertical arrow strokes with the 5th column,
       counting from the left, to align with the center of the "w" glyph
       (U+0077)
     * Aligned most single horizontal arrow strokes with the 7th row,
       counting from the bottom, to align with the horizontal stroke in
       the "e" glyph (U+0065); this follows the convention of Donald Knuth's
       fonts in TeX, as illustrated in The TeXbook
     * Modified the following ranges per the above two re-alignments:
       o U+2190..U+21FF Arrows
       o U+27F0..U+27FF Supplemental Arrows -- A
       o U+2900..U+297F Supplemental Arrows -- B
       o U+2B00..U+2BFF Miscellaneous Symbols and Arrows

   - Modified the following additional Miscellaneous Technical glyphs
     * Scan lines for old 9-line character terminals:
       o U+23BA Line 1, horizontal line across row  1 (counting from the top)
       o U+23BB Line 3, horizontal line across row  5 (counting from the top)
       o U+23BC Line 7, horizontal line across row 12 (counting from the top)
       o U+23BD Line 9, horizontal line across row 16 (counting from the top)
     * U+23CE Return Symbol: shortened to match Latin capital height
     * U+23AF Horizontal Line Extension: aligned on 7th row, counting
       from the bottom
     * U+23D0 Vertical Line Extension: aligned on 5th column, counting
       from the left
     * U+23DA Ground Symbol: aligned with Vertical Line Extension (U+23D0)
     * U+23DB Fuse Symbol: algined with Horizontal Line Extension (U+23AF)
     * U+23EC Black Down-pointing Double Triangle: moved down one row to
       match Latin capital height

   - Swapped U+FE17 and U+FE18, which had been reversed

   - hangul/ directory -- updated "hangul-generation.html" to match the
     latest version at http://unifoundry.com/hangul/hangul-generation.html

Five new utility programs have also been added:

   - unifontpic - creates a bitmapped graphics (.bmp) file of the entire
     Basic Multilingual Plane (Plane 0), by default in a 256-by-256
     glyph grid for ease of printing, and optionally in a 16-by-4096 glyph
     grid for easier scrolling on a screen, for software that can handle
     a .bmp file with over 64k pixel rows (not all software can).  The
     256-by-256 glyph grid can be scaled to print on a piece of paper
     approximately 3 feet by 3 feet (or one meter by one meter).  Written
     by Paul Hardy.

   - unigencircles - adds dashed combining circles to unifont.hex glyphs
     for code points that are in "font/ttfsrc/combining.txt" but not in
     "font/plane00/nonprinting.hex".  Written by Paul Hardy.

   - unigenwidth - creates an implementation of the POSIX functions
     wcwidth() and wcswidth() as specified in IEEE 1003.1-2008, Vol. 2:
     System Interfaces, Issue 7, pages 2251 and 2241, respectively.
     Plane 0 widths are determined by reading the current Unifont glyphs.
     All higher planes, 0x01 through 0x10, are calculated without regard
     to Unifont glyphs.  This can be modified in the future if Unifont
     glyphs extend beyond Plane 0.  Written by Paul Hardy.

   - unihex2png - converts a unifont.hex-format file into a Portable
     Network Graphics (PNG) file for editing with a wider rane of graphics
     editors than the original unihex2bmp allowed.  Written by Andrew
     Miller, based upon the unihex2bmp source code.  Introduced in
     Version 6.3.20131215.

   - unipng2hex - converts a PNG graphics file created by unihex2png
     back into a unifont.hex-format file.  Written by Andrew Miller,
     based upon the unibmp2hex source code.  Introduced in Version
     6.3.20131215.

The last two program additions, unihex2png and unipng2hex, also support
glyph heights of 24 and 32 pixels in addition to Unifont's original
height of 16 pixels.  hex2bdf and hexdraw have also been modified to
support these alternate glyph heights.  This capability has not been
tested extensively, and for now is considered experimental.


CHANGES IN VERSION 6.2
----------------------
After release of version 5.1 of Unifont, it was learned that the
replacement glyphs used in Hangul Syllables, although free to use,
could never be licensed under any version of GPL.  For that reason,
Paul Hardy created a set of Hangul Syllables from scratch with the
oversight of some native Koreans.  This was done using the files that
appear in the "hangul/" directory.  For a detailed discussion of the
process, see

     http://unifoundry.com/hangul/hangul-generation.html

The new font was released as Unifont 6.2, with representation of
all glyphs in the Unicode 6.2 BMP.  As a result of replacing the
Hangul Syllables block, this was the first release that provided
GPLv2+ coverage (with a font embedding exception) for the entire
package.

The Unicode Consortium released Unicode Version 6.2.0 on 22 April 2013.

This version of Unifont includes all additions to the BMP since Unicode
Version 5.1, and adds 1,328 more glyphs to the Basic Multilingual Plane.

It also incorporates all errata that the Unicode Consortium published
that apply to the BMP from Unicode 3.0 errata through Unicode 6.1 errata
(listed with the Unicode 6.2.0 release).  Only one erratum was left
unmodified: the Ogham Space glyph, U+1680, which was left as a line stroke
because of the rendering limitations of the bitmapped Unifont.  The errata
for the following glyphs were examined and if necessary corrected:

     Unicode 3.1: U+066B, U+224C, U+1780..U+17E9
     Unicode 3.2: [none]
     Unicode 4.0: U+06DD, U+0B66
     Unicode 4.1: U+01B3, U+031A
     Unicode 5.0: U+0485, U+0486, U+06E1
     Unicode 5.1: U+047C, U+047D, U+075E, U+075F,
                  U+1031, U+1E9A, U+1460, U+147E,
                  U+2626 
     Unicode 5.2: U+04A8, U+04A9, U+04BE, U+04BF,
                  U+135F, U+19D1, U+19D2, U+19D4 
                  [U+1680 left as is]
     Unicode 6.0: [none]
     Unicode 6.1: U+2D7F

Note that some glyphs were assigned in earlier versions of Unicode and
later withdrawn, but their glyphs still appear in the code charts.
Therefore, they have been left in place.  The Unicode Consortium now
holds the position that once a glyph is assigned, it is not replaced.

Andrew Miller noted that one glyph (U+2047) was incorrect and the glyph
CYRILLIC CAPITAL LETTER A (U+0410) did not match LATIN CAPITAL LETTER A
(U+0041).  He submitted corrections and they have been incorporated.

The biggest change was a totally redrawn set of Hangul Syllables,
U+AC00..U+D7A3, comprising 11,172 glyphs in all.  This allowed the
entire font to be licensed under the GNU GPL.

Unicode 6.2 (and hence Unifont) now only has 2,330 unassigned code points
in the BMP for possible future assignments, and the rate at which new
code points are being assigned in the BMP is decreasing greatly.

The unihex2bmp program has reversed the meaning of its "-f" (flip,
or transpose) flag compared to Unifont Version 5.1 unihex2bmp.
Now the default behavior is to produce 16x16 glyph charts with
the same arrangement as The Unicode Standard.

The unibmp2hex program now hard-codes several scripts and code points
to be double-width.  This was necessary after removing the combining
circles from many glyphs that only occupied the left-hand side of the
16x16 grid, but combine with double-width glyphs from the rest of a
given script.

The "blanks.hex" file has been renamed to "unassigned.hex" as a more
accurate description of its contents.  The "substitutes.hex" file has
been renamed to "spaces.hex", as all it contained were single- and
double-width space glyphs (strings of 0s).


Roman Czyborra and Paul Hardy wanted to license this entire collection
under GPL to simplify its adoption by the GNU Project.  In the end, there
was just one catch: the Hangul Syllables block that appeared in Unifont 5.1,
although licensed for free use, could not be licensed under the GPL.

There was no suitable alternative that was covered under the GPL, so Paul
Hardy created a new block of Hangul Syllables.  This took a few years of
spare time to complete.  Native Koreans reviewed and critiqued the glyphs.
If anyone who is Korean would like to improve this block (U+AC00..U+D7A3),
please feel free to do so and submit the changes so they can be incorporated.

The font has also gone through a couple of simplifications since the
release of version 5.1:

   - There is only one source file for CJK ideographs now, "wqy.hex",
     acknowledging that most of these glyphs were taken from the Wen
     Quan Yi distribution.

   - There are no more combining circles; these were all removed.

The result is now there is just one variation of output font rather than
four.  That one is used to generate the TrueType "unifont.ttf" font.

The directory "font/plane00" contains the .hex input files for building
Unifont, and contains these files:

     hangul-syllables.hex   Unicode Hangul Syllables, U+AC00..U+D7A3
     nonprinting.hex        Format and other assigned but invisible glyphs
     pua.hex                Private Use Area glyphs
     README                 The README file
     spaces.hex             Code points that are space glyphs
     unassigned.hex         Unassigned code points in the BMP
     unifont-base.hex       Source file with almost all BMP scripts
     wqy.hex                Source file with Wen Quan Yi CJK ideographs

The file previously named "blanks.hex" is now named "unassigned.hex".
These "blank" glyphs are no longer included in the compiled font.
Although the Unicode Standard specifically allows a visual rendering
of unassigned code points, doing so would prevent a display engine
finding a glyph in another font.  In fact, the original "blanks.hex"
pattern was modeled after the proposed representation of unassigned
code points depicted in The Unicode Standard, Version 5.0, Section 5.3,
Unknown and Missing Characters (p. 155).

Incorporating "blanks.hex" (now "unassigned.hex") was invaluable in
spotting assigned code points with glyphs that had not yet been drawn.
However, now there is complete coverage of the entire BMP, with only
about 2,300 BMP code points remaining out of 65,536 that could potentially
be given assignments in the future, so the great bulk of work on the
BMP is done.

UNIFONT VERSION 5.1
-------------------
Paul Hardy's first release of Unifont and associated graphics utilities
was Version 5.1.  This corresponded to Unicode Version 5.1 (the current
version at the time), with a glyph for every visible character in the
Unicode 5.1 Basic Multilingual Plane.

For the Unifont 5.1 release, Paul Hardy replaced the 11,172
thick-stroke Hangul Syllables glyphs with thin-stroke glyphs
(a desire expressed by Roman Czyborra for years), merged Qianqian
Fang's unibit and Wen Quan Yi glyphs into GNU Unifont (with lots
of help and enthusiasm from Qianqian Fang), drew about 8,500 more
glyphs to provide complete coerage of the BMP, and replaced the
existing Tibetan glyphs with new ones contributed by Rich Felker.

There was a bug in the johab2ucs2 Perl Script that formed one range
of Hangul Syllables incorrectly in previous releases.  Paul Hardy
noticed and fixed the bug for the Unifont 5.1 release.  All previous
releases of Unifont have an incorrectly formed Hangul Syllables block.

Earlier releases also had an incorrectly formed Braille glyph block.
There was a bug in the Perl script that drew the Braille glyphs in
earlier releases.  Roman Czyborra made a fix to that Perl script,
named "braille" at his website (http://czyborra.com).  The revised
script ("hexbraille") was included in the Unifont 5.1 release, and
used to generate the Unifont 5.1 Braille glyphs.


HISTORY
-------
Roman Czyborra <roman@czybora.com> began GNU Unifont in 1998 as a low
quality font to provide a glyph for every Unicode character in the
Basic Multilingual Plane.  He realized that no one font at the time
had complete coverage of the Unicode BMP.  http://czyborra.com still
has several cool tools for Unifont not included here.

Since Roman Czyborra was unable to maintain the Unifont for a while,
and many patches existed on gnu-unifont@groups.yahoo.com
(http://groups.yahoo.com/group/gnu-unifont), David Starner
<dstarner98@aasaa.ofe.org> decided to make a new release extending
Unifont with many characters in 1999.  That was the foundation of earlier
GNU Unifont compilations from 1999 to 2004.

By 2004, work on Unifont had stopped.  Qianqian Fang wanted to create
a high-quality Chinese Unicode font in 2004.  He began by copying the
GNU Unifont glyphs.  He replaced its Latin glyphs with those of another
X11 font.  He replaced the existing main CJK ideographs with a higher
quality font that the People's Republic of China had placed in the public
domain.  Qianqian named this new font "unibit", and released it under
the terms of the GNU General Public License (GPL) version 2, with the
exception that embedding his font in a document did not by itself bind
that document to the terms of the GNU GPL.

See http://wqy.sourceforge.net/cgi-bin/enindex.cgi (English) or
http://wenq.org (Chinese) for more information on Wen Quan Yi.

In late 2007, Paul Hardy became interested in adding to GNU Unifont.
He wrote a couple of programs to convert GNU Unifont .hex files to and
from bitmap images for easy editing with any graphics software.  He began
by combining the latest glyphs available for GNU Unifont.  This starting
point was posted at http://czyborra.com as the 2007-12-31 version of
unifont.hex.  Shortly after that, Roman Czyborra's website went down.
Paul Hardy then started posting complete copies of GNU Unifont on his
website, at "http://unifoundry.com/unifont.html".

Roman Czyborra encouraged Paul Hardy to continue this work on GNU Unifont.

In early 2008, Paul Hardy learned of Qianqian Fang's work.  Qianqian
encouraged a combining of effort, and Paul Hardy at that point created
two versions of GNU Unifont: one with the original Chinese ideographs
(which Roman Czyborra copied from a Japanese font in the public domain),
and one with Qianqian Fang's Wen Quan Yi (Spring of Letters) ideographs.
The Wen Quan Yi font provides far more coverage of CJK ideographs than
the original Japanese font did, and is of higher quality.

Paul Hardy created a version of both the font with the original CJK
ideographs from Japan and with CJK ideographs from Wen Quan Yi that
contained combining circles.  He then wrote a post-processing program
to remove the combining circles from the final font.

In 2005, Luis Alejandro Gonzalez Miranda (http://www.lgm.cl) created
a set of Fontforge scripts and Perl programs to build a TrueType font
from unifont.hex.  Paul Hardy modified Luis' software in 2008 to cover
the full Unicode 5.1 Basic Multilingual Plane range.  Luis gave Paul
Hardy permission to release this modified version under the terms of
"the GNU General Public License, version 2 or (at your option) a later
version."

On 4 July 2008, Paul Hardy was looking through all of Roman Czyborra's
Perl scripts.  One of these, "braille", contained a comment from 2003
that the original GNU Unifont did not generate its Braille patterns
(U+2800..U+28FF) correctly.  The modified script fixed that bug.  Paul
Hardy incorporated the corrected Braille glyphs into the 6 July 2008
release of GNU Unifont.

All previous versions probably contain this Braille bug and should be
replaced.

Other notable additions include:

     - Incorporation of CJK glyphs from Qianqian Fang's fonts

     - Incorporation of Rich Felker's Tibetan glyphs

     - Replacement of the Hangul Syllables block with a thin stroke font
       (Roman had mentioned wanting to do this someday on his website),
       the current version being created from scratch by Paul Hardy

     - Addition of circled pencil glyphs for the Private Use Area
       (suggested as an acceptable rendering in the Unicode 5.0 Standard),
       now replaced with optional four-digit hexadecimal code point glyphs;
       thought not built into the final font by default, they are available
       in "font/plane00/pua.hex"

     - Replacement of the Unifont 5.1 gray box glyphs for unassigned
       code points with four-digit hexadecimal glyphs; these are built
       into the final font by default

     - Proper handling of combining characters in the TrueType version

     - Proper handling of space glyphs in the TrueType version

The hex2bdf script in this release is Roman's original script, not the
modified version that produced two BDF files (one for 8 pixel wide glyphs
and another for 16 pixel wide glyphs).  The TrueType font should be used
in preference to the BDF font, so this is probably a moot point.

For the Unifont 6.2 release, Qianqian Fang gave Paul Hardy permission
to release the subset of Wen Quan Yi glyphs included in Unifont under
GPLv2+, with a font embedding exception.  With the newly-drawn Hangul
Syllables block, this allowed the entire font to be released under
GPLv2+ with a font embedding exception.


OPEN ISSUES
-----------
* Some CJK ideographs use an entire 16x16 pixel grid.  This leaves
insufficient space between lines.  However, changing to a non-square
grid would distort the block drawing glyphs.  The best solution is
probably to use GNU Unifont for mostly non-CJK glyph rendering, and
to use Qianqian Fang's Wen Quan Yi fonts (http://wenq.org) for
predominately CJK glyph rendering.  The Wen Quan Yi fonts use extra
leading (blank space) between lines.

* There are still some Control and Format glyphs in "unifont-base.hex";
these might be more appropriate for "nonprinting.hex".


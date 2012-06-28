In order to build the bossac tool, check out the BOSSA repository at Sourceforge
as follows:

git clone git://git.code.sf.net/p/b-o-s-s-a/code b-o-s-s-a-code

Apply the patch with the following command

patch -d b-o-s-s-a-code -p1 < bossac.patch

To build bossac, change to the bossac source directory and run

make bin/bossac

The resulting binary will be at bin/bossac.

NOTE: the patch will cleanly apply to change 05bfcc39bc0453c3028b1161175b95a81af7a901.
Subsequent commits on the bossa repository may implement the patch, and render
it obsolete.

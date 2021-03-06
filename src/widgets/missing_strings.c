/*
 * "$Id: flstring.c 4288 2005-04-16 00:13:17Z mike $"
 *
 * missing BSD string functions for the Fast Light Tool Kit (FLTK).
 * version < 1.3.2
 *
 * Copyright 1998-2005 by Bill Spitzak and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 *
 *  Free Software Foundation, Inc.
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301 USA.
 *
 * Please report all bugs and problems on the following page:
 *
 *     http://www.fltk.org/str.php
 */

#include "missing_strings.h"

// versions of FLTK < 1.3.2 do not contain fl_string
#if (FLCLUSTER_FLTK_API_MAJOR == 1 && FLCLUSTER_FLTK_API_MINOR == 3 && FLCLUSTER_FLTK_API_PATCH < 1)

/*
 * 'fl_strlcat()' - Safely concatenate two strings.
 */

size_t				/* O - Length of string */
fl_strlcat(char       *dst,	/* O - Destination string */
		   const char *src,	/* I - Source string */
		   size_t     size) {	/* I - Size of destination string buffer */
	size_t	srclen;		/* Length of source string */
	size_t	dstlen;		/* Length of destination string */


	/*
	 * Figure out how much room is left...
	 */

	dstlen = strlen(dst);
	size   -= dstlen + 1;

	if (!size) return (dstlen);	/* No room, return immediately... */

	/*
	 * Figure out how much room is needed...
	 */

	srclen = strlen(src);

	/*
	 * Copy the appropriate amount...
	 */

	if (srclen > size) srclen = size;

	memcpy(dst + dstlen, src, srclen);
	dst[dstlen + srclen] = '\0';

	return (dstlen + srclen);
}


/*
 * 'fl_strlcpy()' - Safely copy two strings.
 */

size_t				/* O - Length of string */
fl_strlcpy(char       *dst,	/* O - Destination string */
		   const char *src,	/* I - Source string */
		   size_t      size) {	/* I - Size of destination string buffer */
	size_t	srclen;		/* Length of source string */


	/*
	 * Figure out how much room is needed...
	 */

	size --;

	srclen = strlen(src);

	/*
	 * Copy the appropriate amount...
	 */

	if (srclen > size) srclen = size;
	
	memcpy(dst, src, srclen);
	dst[srclen] = '\0';
	
	return (srclen);
}

#endif

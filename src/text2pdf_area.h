/*
 ** Copyright (©) 2003-2008 Teus Benschop.
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */

#ifndef INCLUDED_TEXT2PDF_AREA_H
#define INCLUDED_TEXT2PDF_AREA_H

#include "libraries.h"
#include <pango/pangocairo.h>

class T2PArea
{
public:
  T2PArea(PangoRectangle rectangle_in);
  virtual ~T2PArea();
  PangoRectangle rectangle;
  int baseline_offset_pango_units;
  ustring rectangle2text ();
private:
};

#endif

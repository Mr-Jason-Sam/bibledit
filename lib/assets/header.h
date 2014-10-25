/*
Copyright (©) 2003-2014 Teus Benschop.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef INCLUDED_ASSETS_HEADER_H
#define INCLUDED_ASSETS_HEADER_H


#include <config/libraries.h>
#include <assets/view.h>


class Assets_Header
{
public:
  Assets_Header (string title);
  ~Assets_Header ();
  void jQueryUIOn (string subset = "");
  void addHeadLine (string line);
  void setBodyOnload (string code);
  void setLogin ();
  void setNavigator ();
  void setStylesheet ();
  void setEditorStylesheet ();
  void setSearchQuery (string query);
  bool displayTopbar ();
  string run ();
private:
  Assets_View * view;
  bool includeJQueryUI;
  string JQueryUISubset;
  vector <string> headLines;
  bool displayNavigator;
  bool includedStylesheet;
  bool includedEditorStylesheet;
};


#endif

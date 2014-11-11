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


#ifndef INCLUDED_FILTER_USFM_H
#define INCLUDED_FILTER_USFM_H


#include <config/libraries.h>


class BookChapterData
{
public:
  BookChapterData (int book_in, int chapter_in, string data_in);
  int book;
  int chapter;
  string data;
};


class UsfmNote
{
public:
  UsfmNote (int offset_in, string data_in);
  int offset;
  string data;
};


string usfm_one_string (string usfm);
vector <string> usfm_get_markers_and_text (string code);
string usfm_get_marker (string usfm);
vector <BookChapterData> usfm_import (string input, string stylesheet);
vector <int> usfm_get_verse_numbers (string usfm);
int usfm_linenumber_to_versenumber (string usfm, unsigned int line_number);
int usfm_offset_to_versenumber (string usfm, unsigned int offset);
int usfm_versenumber_to_offset (string usfm, int verse);
string usfm_get_verse_text (string usfm, int verse_number);
bool usfm_is_usfm_marker (string code);
bool usfm_is_opening_marker (string usfm);
bool usfm_is_embedded_marker (string usfm);
string usfm_get_book_identifier (const vector <string>& usfm, unsigned int pointer);
string usfm_get_text_following_marker (const vector <string>& usfm, unsigned int & pointer);
string usfm_peek_text_following_marker (const vector <string>& usfm, unsigned int pointer);
string usfm_peek_verse_number (string usfm);
string usfm_get_opening_usfm (string text, bool embedded = false);
string usfm_get_closing_usfm (string text, bool embedded = false);
vector <UsfmNote> usfm_extract_notes (string usfm, const vector <string> & markers);


#endif

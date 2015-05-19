/*
 Copyright (©) 2003-2015 Teus Benschop.
 
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


#include <edit/offset.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/usfm.h>
#include <webserver/request.h>
#include <ipc/focus.h>
#include <editor/import.h>


string edit_offset_url ()
{
  return "edit/offset";
}


bool edit_offset_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::translator ());
}


string edit_offset (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  
  string bible = request->query ["bible"];
  int book = convert_to_int (request->query ["book"]);
  int chapter = convert_to_int (request->query ["chapter"]);
  
  
  // At first the browser used the rangy library to get the offset of the caret.
  // But the rangy library provides the offset relative to the element that contains the caret,
  // not relative to the main editor element.
  // Therefore a pure Javascript implementation was Googled for and implemented.
  // This provides the offset of the caret relative to the <div id="editor">.
  size_t offset = convert_to_int (request->query ["offset"]);

  
  string stylesheet = request->database_config_user()->getStylesheet ();
  string usfm = request->database_bibles()->getChapter (bible, book, chapter);
  
  
  Editor_Import editor_import = Editor_Import (request);
  editor_import.load (usfm);
  editor_import.stylesheet (stylesheet);
  editor_import.run ();

  
  int verse = -1;

  
  // The caret offset should be in the main text body.
  // If it is in a note body, skip the verse updating.
  if (offset <= editor_import.textLength) {
    // Look for the verse that matches the offset.
    for (auto & item : editor_import.verseStartOffsets) {
      int key = item.first;
      size_t value = item.second;
      if (offset >= value) {
        // A verse number was found.
        verse = key;
      }
    }
  }
  

  // Only act if a verse was found
  if (verse >= 0) {
    // Only update navigation in case the verse changed.
    // This avoids unnecessary focus operations in the clients.
    if (verse != Ipc_Focus::getVerse (request)) {
      Ipc_Focus::set (request, book, chapter, verse);
    }
    // The editor should scroll the verse into view,
    // because the caret is in the Bible text.
    return convert_to_string (verse);
    // If the caret were in the notes area,
    // then the editor should not scroll the verse into view.
  }
  
  
  return "";
}
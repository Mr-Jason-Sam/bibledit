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


#include <editusfm/index.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/header.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/customcss.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <access/bible.h>
#include <database/config/bible.h>
#include <fonts/logic.h>
#include <navigation/passage.h>
#include <dialog/list.h>
#include <ipc/focus.h>


string editusfm_index_url ()
{
  return "editusfm/index";
}


bool editusfm_index_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::translator ());
}


string editusfm_index (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  
  if (request->query.count ("switchbook") && request->query.count ("switchchapter")) {
    string switchbook = request->query ["switchbook"];
    string switchchapter = request->query ["switchchapter"];
    int book = convert_to_int (switchbook);
    int chapter = convert_to_int (switchchapter);
    Ipc_Focus::set (request, book, chapter, 1);
    Navigation_Passage::recordHistory (request, book, chapter, 1);
  }
  
  
  string page;
  
  Assets_Header header = Assets_Header (gettext("Edit USFM"), request);
  header.setNavigator ();
  page = header.run ();
  
  if (request->query.count("changebible")) {
    string changebible = request->query["changebible"];
    if (changebible == "") {
      Dialog_List dialog_list = Dialog_List ("index", gettext("Select which Bible to open in the editor"), "", "");
      vector <string> bibles = access_bible_bibles (request);
      for (auto & bible : bibles) {
        dialog_list.add_row (bible, "changebible", bible);
      }
      page += dialog_list.run();
      return page;
    } else {
      request->database_config_user()->setBible (changebible);
    }
  }

  
  Assets_View view = Assets_View ();

  
  // Get active Bible, and check read access to it.
  // If needed, change Bible to one it has read access to.
  string bible = access_bible_clamp (request, request->database_config_user()->getBible ());
  view.set_variable ("bible", bible);
  
  
  // Write access?
  bool write_access = access_bible_write (request, bible);
  view.set_variable ("write_access", write_access ? "true" : "false");
  
  
  // Store the active Bible in the page's javascript.
  view.set_variable ("navigationCode", Navigation_Passage::code (bible));
  
  
  string chapterLoaded = gettext("Loaded");
  string chapterSaving = gettext("Saving...");
  string chapterRetrying = gettext("Retrying...");
  string s_write_access = write_access ? "true" : "false";
  string script =
  "var usfmEditorChapterLoaded = \"" + chapterLoaded + "\";\n"
  "var usfmEditorChapterSaving = \"" + chapterSaving + "\";\n"
  "var usfmEditorChapterRetrying = \"" + chapterRetrying + "\";\n"
  "var usfmEditorWriteAccess = " + s_write_access + ";\n";
  view.set_variable ("script", script);
  

  string cls = Filter_CustomCSS::getClass (bible);
  string font = Database_Config_Bible::getTextFont (bible);
  int direction = Database_Config_Bible::getTextDirection (bible);
  view.set_variable ("custom_class", cls);
  view.set_variable ("custom_css", Filter_CustomCSS::getCss (cls, Fonts_Logic::getFontPath (font), direction));
  
  
  page += view.render ("editusfm", "index");
  
  
  page += Assets_Page::footer ();
  
  
  return page;
}


// Tests for the USFM editor:
// * Autosave on going to another passage.
// * Autosave on document unload.
// * Autosave shortly after any change.
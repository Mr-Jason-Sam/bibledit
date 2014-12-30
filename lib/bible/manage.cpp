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


#include <bible/manage.h>
#include <assets/view.h>
#include <assets/page.h>
#include <filter/roles.h>
#include <webserver/request.h>
#include <database/versifications.h>
#include <locale/translate.h>
#include <dialog/entry.h>
#include <dialog/yes.h>
#include <access/bible.h>
#include <bible/logic.h>


string bible_manage_url ()
{
  return "bible/manage";
}


bool bible_manage_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::manager ());
}


string bible_manage (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  string page;
  
  page = Assets_Page::header (gettext ("Bibles"), webserver_request, "");
  
  Assets_View view = Assets_View ();
  
  string success_message;
  string error_message;

  // New Bible handler.
  if (request->query.count ("new")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("manage", gettext("Please enter a name for the new empty Bible"), "", "new", "");
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("new")) {
    string bible = request->post ["entry"];
    vector <string> bibles = request->database_bibles ()->getBibles ();
    if (find (bibles.begin(), bibles.end(), bible) != bibles.end()) {
      error_message = gettext("This Bible already exists");
    } else {
      request->database_bibles ()->createBible (bible);
      // Check / grant access.
      if (!access_bible_write (request, bible)) {
        request->database_users ()->grantAccess2Bible (request->session_logic ()->currentUser (), bible);
      }
      success_message = gettext("The Bible was created");
    }
  }
  
  // Copy Bible handler.
  if (request->query.count ("copy")) {
    string copy = request->query["copy"];
    Dialog_Entry dialog_entry = Dialog_Entry ("manage", gettext("Please enter a name for where to copy the Bible to"), "", "", "A new Bible will be created with the given name, and the current Bible copied to it");
    dialog_entry.add_query ("origin", copy);
    page += dialog_entry.run ();
    return page;
  }
  if (request->query.count ("origin")) {
    string origin = request->query["origin"];
    if (request->post.count ("entry")) {
      string destination = request->post["entry"];
      vector <string> bibles = request->database_bibles ()->getBibles ();
      if (find (bibles.begin(), bibles.end(), destination) != bibles.end()) {
        error_message = gettext("Cannot copy the Bible because the destination Bible already exists.");
      } else {
        // User needs read access to the original.
        if (access_bible_read (request, origin)) {
          request->database_bibles ()->createBible (destination);
          vector <int> books = request->database_bibles ()->getBooks (origin);
          for (auto & book : books) {
            vector <int> chapters = request->database_bibles ()->getChapters (origin, book);
            for (auto & chapter : chapters) {
              string data = request->database_bibles ()->getChapter (origin, book, chapter);
              Bible_Logic::storeChapter (destination, book, chapter, data);
            }
          }
          success_message = gettext("The Bible was copied.");
          // Check / grant access to destination Bible.
          if (!access_bible_write (request, destination)) {
            request->database_users ()->grantAccess2Bible (request->session_logic ()->currentUser (), destination);
          }
        }
      }
    }
  }

  // Delete Bible handler.
  if (request->query.count ("delete")) {
    string bible = request->query ["delete"];
    string confirm = request->query ["confirm"];
    if (confirm == "yes") {
      // User needs write access for delete operation.
      if (access_bible_write (request, bible)) {
        Bible_Logic::deleteBible (bible);
        /* C++Port
        string gitdirectory = Filter_Git::git_directory (bible);
        if (file_exists (gitdirectory)) {
          Filter_Rmdir::rmdir (gitdirectory);
        }
        */
      } else {
        page += Assets_Page::error ("Insufficient privileges to complete action");
      }
    }
    if (confirm == "") {
      Dialog_Yes dialog_yes = Dialog_Yes ("manage", gettext("Would you like to delete this Bible?") + " (" + bible + ")");
      dialog_yes.add_query ("delete", bible);
      page += dialog_yes.run ();
      return page;
    }
  }

  view.set_variable ("success_message", success_message);
  view.set_variable ("error_message", error_message);
  vector <string> bibles = access_bible_bibles (request);
  string bibleblock;
  for (auto & bible : bibles) {
    bibleblock.append ("<li><a href=\"settings?bible=" + bible + "\">" + bible + "</a></li>\n");
  }
  view.set_variable ("bibleblock", bibleblock);
                       

  page += view.render ("bible", "manage");
  
  page += Assets_Page::footer ();
  
  return page;
}

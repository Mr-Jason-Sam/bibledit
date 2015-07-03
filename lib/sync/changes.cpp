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


#include <sync/changes.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/modifications.h>
#include <client/logic.h>
#include <locale/translate.h>
#include <webserver/request.h>
#include <sync/logic.h>
#include <checksum/logic.h>


string sync_changes_url ()
{
  return "sync/changes";
}


bool sync_changes_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string sync_changes (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  Sync_Logic sync_logic = Sync_Logic (webserver_request);
  Database_Modifications database_modifications;

  // Check on the credentials.
  if (!sync_logic.credentials_okay ()) return "";

  // Get the relevant parameters the client may have POSTed to us, the server.
  string user = hex2bin (request->post ["u"]);
  int action = convert_to_int (request->post ["a"]);
  int id = convert_to_int (request->post ["i"]);

  switch (action) {
    case Sync_Logic::changes_delete_modification:
    {
      // The server deletes the change notification.
      database_modifications.deleteNotification (id);
      Database_Logs::log ("Client deletes change notification from server: " + convert_to_string (id), Filter_Roles::translator ());
      return "";
    }
    case Sync_Logic::changes_get_checksum:
    {
      // The server responds with the total checksum for the user's change notifications.
      return Sync_Logic::changes_checksum (user);
    }
    case Sync_Logic::changes_get_identifiers:
    {
      // The server responds with the identifiers of all the user's change notifications.
      vector <int> ids = database_modifications.getNotificationIdentifiers (user, false);
      string response;
      for (auto & id : ids) {
        if (!response.empty ()) response.append ("\n");
        response.append (convert_to_string (id));
      }
      return response;
    }
    case Sync_Logic::changes_get_modification:
    {
      // The server responds with the relevant data of the requested modification.
      vector <string> lines;
      // category
      lines.push_back (database_modifications.getNotificationCategory (id));
      // bible
      lines.push_back (database_modifications.getNotificationBible (id));
      // book
      // chapter
      // verse
      Passage passage = database_modifications.getNotificationPassage (id);
      lines.push_back (convert_to_string (passage.book));
      lines.push_back (convert_to_string (passage.chapter));
      lines.push_back (passage.verse);
      // oldtext
      lines.push_back (database_modifications.getNotificationOldText (id));
      // modification
      lines.push_back (database_modifications.getNotificationModification (id));
      // newtext
      lines.push_back (database_modifications.getNotificationNewText (id));
      // Result.
      return filter_string_implode (lines, "\n");
    }
  }

  
  // Bad request.
  // Delay a while to obstruct a flood of bad requests.
  this_thread::sleep_for (chrono::seconds (1));
  request->response_code = 400;
  return "";
}

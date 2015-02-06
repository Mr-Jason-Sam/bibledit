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


#include <sendreceive/settings.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/bibles.h>
#include <client/logic.h>
#include <locale/translate.h>
#include <webserver/request.h>
#include <sync/logic.h>
#include <sync/settings.h>


void sendreceive_settings ()
{
  Database_Logs::log (gettext("Settings: Send/Receive"), Filter_Roles::translator ());
  
  Webserver_Request request;
  Sync_Logic sync_logic = Sync_Logic (&request);

  string response = client_logic_connection_setup ();
  int iresponse = convert_to_int (response);
  if (iresponse < Filter_Roles::guest () || iresponse > Filter_Roles::admin ()) {
    Database_Logs::log (gettext("Failure sending and receiving Settings"), Filter_Roles::translator ());
    return;
  }

  // Set the correct user in the session: The sole user on the Client.
  vector <string> users = request.database_users ()->getUsers ();
  if (users.empty ()) {
    Database_Logs::log (gettext("No user found"), Filter_Roles::translator ());
    return;
  }
  string user = users [0];
  request.session_logic ()->setUsername (user);
  
  string address = Database_Config_General::getServerAddress ();
  int port = Database_Config_General::getServerPort ();
  string url = client_logic_url (address, port, sync_settings_url ());
  
  // Go through all settings flagged as having been updated on this client.
  vector <int> ids = request.database_config_user()->getUpdatedSettings ();
  if (!ids.empty ()) {
    Database_Logs::log (gettext("Sending settings"), Filter_Roles::translator ());
  }
  
  // The POST request contains the credentials.
  map <string, string> post;
  post ["u"] = bin2hex (user);
  post ["p"] = request.database_users ()->getmd5 (user);
  post ["l"] = convert_to_string (request.database_users ()->getUserLevel (user));

  for (auto id : ids) {

    // What to request for.
    post ["a"] = convert_to_string (id);

    string value;
    switch (id) {
      case Sync_Logic::settings_send_workbench_urls:
        value = request.database_config_user()->getWorkbenchURLs ();
        break;
      case Sync_Logic::settings_send_workbench_widths:
        value = request.database_config_user()->getWorkbenchWidths ();
        break;
      case Sync_Logic::settings_send_workbench_heights:
        value = request.database_config_user()->getWorkbenchHeights ();
        break;
    }
    post ["v"] = value;
    
    // POST the setting to the server.
    string error;
    sync_logic.post (post, url, error);
    
    // Handle server's response.
    if (!error.empty ()) {
      Database_Logs::log ("Failure sending setting to server", Filter_Roles::translator ());
    } else {
      request.database_config_user()->removeUpdatedSetting (id);
    }
  }

  // All changed settings have now been sent to the server.
  // The client will now synchronize its settings with the server's settings.

  // The script requests the checksum of all relevant settings from the server.
  // It compares this with the local checksum.
  // If it matches, that means that the local settings match the settings on the server.
  // The script is then ready.
  if (post.count ("v")) post.erase (post.find ("v"));
  post ["a"] = convert_to_string (Sync_Logic::settings_get_total_checksum);
  string error;
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log ("Failure synchronizing Settings while requesting totals", Filter_Roles::translator ());
    return;
  }
  string checksum = sync_logic.settings_checksum ();
  if (response == checksum) {
    Database_Logs::log ("Settings: Up to date", Filter_Roles::translator ());
    return;
  }

  // At this stage the total checksum of all relevant settings on the client differs from the same on the server.
  // Requests all settings from the server.

  post ["a"] = convert_to_string (Sync_Logic::settings_get_workbench_urls);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log ("Failure receiving workbench URLS", Filter_Roles::translator ());
    return;
  }
  request.database_config_user()->setWorkbenchURLs (response);

  post ["a"] = convert_to_string (Sync_Logic::settings_get_workbench_widths);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log ("Failure receiving workbench widths", Filter_Roles::translator ());
    return;
  }
  request.database_config_user()->setWorkbenchWidths (response);

  post ["a"] = convert_to_string (Sync_Logic::settings_get_workbench_heights);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log ("Failure receiving workbench heights", Filter_Roles::translator ());
    return;
  }
  request.database_config_user()->setWorkbenchHeights (response);
  
  // Done.
  Database_Logs::log ("Settings: Ready", Filter_Roles::translator ());
}

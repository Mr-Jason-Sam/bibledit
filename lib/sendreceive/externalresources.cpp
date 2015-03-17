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


#include <sendreceive/externalresources.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/merge.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <client/logic.h>
#include <locale/translate.h>
#include <webserver/request.h>
#include <sync/logic.h>
#include <sync/externalresources.h>
#include <checksum/logic.h>
#include <database/logs.h>
#include <database/offlineresources.h>
#include <database/config/general.h>
#include <database/books.h>


mutex mutex_sendreceive_externalresources;
bool sendreceive_externalresources_running = false;


void sendreceive_externalresources ()
{
  mutex_sendreceive_externalresources.lock ();
  bool bail_out = sendreceive_externalresources_running;
  mutex_sendreceive_externalresources.unlock ();
  if (bail_out) return;
  mutex_sendreceive_externalresources.lock ();
  sendreceive_externalresources_running = true;
  mutex_sendreceive_externalresources.unlock ();
  
  
  Database_OfflineResources database_offlineresources = Database_OfflineResources ();
  Webserver_Request request;
  Sync_Logic sync_logic = Sync_Logic (&request);

  
  Database_Logs::log (translate("External resources: Synchronizing"), Filter_Roles::translator ());
  
  
  string address = Database_Config_General::getServerAddress ();
  int port = Database_Config_General::getServerPort ();
  string url = client_logic_url (address, port, sync_externalresources_url ());
  
  
  map <string, string> post;
  string error;
  string response;
  
  
  // Request the checksum of all offline resources from the server.
  // Compare it with the local checksum.
  // If the two match: Ready.
  post ["a"] = to_string (Sync_Logic::offlineresources_get_total_checksum);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log ("External resources: Failure requesting checksum: " + error, Filter_Roles::translator ());
    return;
  }
  string checksum = Sync_Logic::offline_resources_checksum ();
  if (response == checksum) {
    Database_Logs::log ("External resources: Up to date", Filter_Roles::translator ());
    return;
  }
  
  
  // Request a list of all offline resources available on the server.
  post ["a"] = to_string (Sync_Logic::offlineresources_get_resources);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log ("External resources: Failure requesting names: " + error, Filter_Roles::translator ());
    return;
  }
  vector <string> server_resources = filter_string_explode (response, '\n');

  
  // Delete resources that exist locally but not on the server.
  vector <string> client_resources = database_offlineresources.names ();
  vector <string> resources = filter_string_array_diff (client_resources, server_resources);
  for (auto & resource : resources) {
    database_offlineresources.erase (resource);
  }
  
  
  // Deal with each offline resource individually.
  for (auto & resource : server_resources) {
    
    
    // Request the checksum of the resources from the server.
    // Compare it with the checksum of the local resource.
    // If they match: Go to next resource.
    post ["a"] = to_string (Sync_Logic::offlineresources_get_resource_checksum);
    post ["r"] = resource;
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log ("External resources: Failure requesting checksum: " + error, Filter_Roles::translator ());
      return;
    }
    checksum = Sync_Logic::offline_resource_checksum (resource);
    if (response == checksum) {
      continue;
    }

    
    // Request a list of all files in the resource on the server.
    post ["a"] = to_string (Sync_Logic::offlineresources_get_files);
    post ["r"] = resource;
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log ("External resources: Failure requesting files: " + error, Filter_Roles::translator ());
      return;
    }
    vector <string> server_files = filter_string_explode (response, '\n');
  
    
    // Delete files that exist locally but not on the server.
    vector <string> client_files = database_offlineresources.files (resource);
    vector <string> files = filter_string_array_diff (client_files, server_files);
    for (auto & file : files) {
      database_offlineresources.unlink (resource, file);
    }
    
    
    // Deal with each file individually.
    for (auto & file : server_files) {
      
      
      // Request checksum of this file,
      // compare it with the local checksum,
      // and skip the file if the checksums match.
      post ["a"] = to_string (Sync_Logic::offlineresources_get_file_checksum);
      post ["r"] = resource;
      post ["f"] = file;
      response = sync_logic.post (post, url, error);
      if (!error.empty ()) {
        Database_Logs::log ("External resources: Failure requesting checksum file: " + error, Filter_Roles::translator ());
        return;
      }
      checksum = Sync_Logic::offline_resource_file_checksum (resource, file);
      if (response == checksum) {
        continue;
      }

      
      // Download the file from the server, and store it locally on the client.
      Database_Logs::log ("External resources: Downloading " + resource + " " + file, Filter_Roles::translator ());
      // Create directory by saving empty file.
      database_offlineresources.save (resource, file, "");
      // Obtain url for file to download from server.
      post ["a"] = to_string (Sync_Logic::offlineresources_get_file_filename);
      post ["r"] = resource;
      post ["f"] = file;
      response = sync_logic.post (post, url, error);
      if (!error.empty ()) {
        Database_Logs::log ("External resources: Failure downloading file: " + error, Filter_Roles::translator ());
        return;
      }
      string url = client_logic_url (address, port, response);
      // Local file path where to save resource.
      string filepath = database_offlineresources.filepath (resource, file);
      // Download and save file locally.
      filter_url_download_file (url, filepath, error);
      if (!error.empty ()) {
        Database_Logs::log ("External resources: Failure downloading file: " + error, Filter_Roles::translator ());
        return;
      }
    }
  }
  
  
  // Done.
  Database_Logs::log ("External resources: Now up to date", Filter_Roles::translator ());

  
  mutex_sendreceive_externalresources.lock ();
  sendreceive_externalresources_running = false;
  mutex_sendreceive_externalresources.unlock ();
}

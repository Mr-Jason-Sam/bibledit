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


#include <sendreceive/notes.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/merge.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/notes.h>
#include <database/noteactions.h>
#include <database/logs.h>
#include <client/logic.h>
#include <locale/translate.h>
#include <webserver/request.h>
#include <sync/logic.h>
#include <sync/notes.h>
#include <checksum/logic.h>
#include <bible/logic.h>
#include <notes/logic.h>


mutex mutex_sendreceive_notes;
bool sendreceive_notes_running = false;


void sendreceive_notes_done ()
{
  mutex_sendreceive_notes.lock ();
  sendreceive_notes_running = false;
  mutex_sendreceive_notes.unlock ();
}


string sendreceive_notes_text ()
{
  return translate("Notes") + ": ";
}


string sendreceive_notes_sendreceive_text ()
{
  return sendreceive_notes_text () + translate ("Send/receive");
}


string sendreceive_notes_up_to_date_text ()
{
  return sendreceive_notes_text () + translate ("Up to date");
}


void sendreceive_notes ()
{
  mutex_sendreceive_notes.lock ();
  bool bail_out = sendreceive_notes_running;
  mutex_sendreceive_notes.unlock ();
  if (bail_out) return;
  mutex_sendreceive_notes.lock ();
  sendreceive_notes_running = true;
  mutex_sendreceive_notes.unlock ();
  
  
  Webserver_Request request;
  Sync_Logic sync_logic = Sync_Logic (&request);
  Database_Notes database_notes = Database_Notes (&request);
  Database_NoteActions database_noteactions = Database_NoteActions ();
  Notes_Logic notes_logic = Notes_Logic (&request);
  
  
  Database_Logs::log (sendreceive_notes_sendreceive_text (), Filter_Roles::translator ());
  
  
  string response = client_logic_connection_setup ();
  int iresponse = convert_to_int (response);
  if (iresponse < Filter_Roles::guest () || iresponse > Filter_Roles::admin ()) {
    Database_Logs::log (sendreceive_notes_text () + translate("Failure to initiate connection"), Filter_Roles::translator ());
    sendreceive_notes_done ();
    return;
  }
  
  
  // Set the correct user in the session: The sole user on the Client.
  vector <string> users = request.database_users ()->getUsers ();
  if (users.empty ()) {
    Database_Logs::log (sendreceive_notes_text () + translate("No local user found"), Filter_Roles::translator ());
    sendreceive_notes_done ();
    return;
  }
  string user = users [0];
  request.session_logic ()->setUsername (user);
  
  
  // The basic request to be POSTed to the server.
  // It contains the user's credentials.
  map <string, string> post;
  post ["u"] = bin2hex (user);
  post ["p"] = request.database_users ()->getmd5 (user);
  post ["l"] = convert_to_string (request.database_users ()->getUserLevel (user));

  
  // Error variable.
  string error;
  
  
  // Server URL to call.
  string address = Database_Config_General::getServerAddress ();
  int port = Database_Config_General::getServerPort ();
  string url = client_logic_url (address, port, sync_notes_url ());
  
  
  // Check on communication errors to be careful that there will be no loss of notes data on the client.
  // While sending updates for a certai note identifier,
  // make sure to take the most recent update if there's more than one in the database for that identifier.

  
  // Go through all notes which have actions recorded for them.
  // While sending note actions, the database method to retrieve them
  // keeps the sequence of the actions as they occurred,
  // as later updates can undo or affect earlier updates.
  vector <int> notes = database_noteactions.getNotes ();
  for (auto identifier : notes) {
  
  
    string summary = database_notes.getSummary (identifier);
    if (summary.empty ()) summary = "<deleted>";
    Database_Logs::log (sendreceive_notes_text () + translate("Sending note to server") + ": " + summary, Filter_Roles::translator ());

    
    // Get all the actions for the current note.
    vector <Database_Note_Action> note_actions = database_noteactions.getNoteData (identifier);
    
    
    // Due to some updates sent out here, record final actions to get the updated note from the server.
    map <int, bool> final_get_actions;

    
    // Deal with the note actions for this note.
    for (auto note_action : note_actions) {
      
      int rowid = note_action.rowid;
      string username = note_action.username;
      //int timestamp = note_action.timestamp;
      int action = note_action.action;
      string content = note_action.content;
      
      
      // Generate a POST request.
      post ["i"] = convert_to_string (identifier);
      post ["a"] = convert_to_string (action);
      switch (action) {
        case Sync_Logic::notes_put_create_initiate: break;
        case Sync_Logic::notes_put_create_complete: break;
        case Sync_Logic::notes_put_summary:
        {
          content = database_notes.getSummary (identifier);
          break;
        }
        case Sync_Logic::notes_put_contents: break;
        case Sync_Logic::notes_put_comment: break;
        case Sync_Logic::notes_put_subscribe: break;
        case Sync_Logic::notes_put_unsubscribe: break;
        case Sync_Logic::notes_put_assign: break;
        case Sync_Logic::notes_put_unassign: break;
        case Sync_Logic::notes_put_status:
        {
          content = database_notes.getRawStatus (identifier);
          break;
        }
        case Sync_Logic::notes_put_passages:
        {
          vector <Passage> passages = database_notes.getPassages (identifier);
          vector <string> lines;
          for (auto & passage : passages) {
            lines.push_back  convert_to_string (filter_passage_to_integer (passage)));
          }
          content = filter_string_implode (lines, "\n");
          break;
        }
        case Sync_Logic::notes_put_severity:
        {
          content = convert_to_string (database_notes.getRawSeverity (identifier));
          break;
        }
        case Sync_Logic::notes_put_bible:
        {
          content = database_notes.getBible (identifier);
          break;
        }
        case Sync_Logic::notes_put_mark_delete: break;
        case Sync_Logic::notes_put_unmark_delete: break;
        case Sync_Logic::notes_put_delete: break;
      }
      post ["c"] = content;

      
      // Send the request off and receive the response.
      response = sync_logic.post (post, url, error);
      if (!error.empty ()) {
        Database_Logs::log (sendreceive_notes_text () + "Failure sending note: " + error, Filter_Roles::translator ());
        sendreceive_notes_done ();
        return;
      }

      
      // Delete this note action because it has been dealt with.
      database_noteactions.erase (rowid);
      

      // Add final note actions depending on what was updated.
      switch (action) {
        case Sync_Logic::notes_put_create_initiate: break;
        case Sync_Logic::notes_put_create_complete:
        {
          int i = Sync_Logic::notes_get_contents;
          final_get_actions [i] = true;
          i = Sync_Logic::notes_get_subscribers;
          final_get_actions [i] = true;
          i = Sync_Logic::notes_get_assignees;
          final_get_actions [i] = true;
          i = Sync_Logic::notes_get_modified;
          final_get_actions [i] = true;
          break;
        }
        case Sync_Logic::notes_put_summary: break;
        case Sync_Logic::notes_put_contents: break;
        case Sync_Logic::notes_put_comment:
        {
          int i = Sync_Logic::notes_get_contents;
          final_get_actions [i] = true;
          i = Sync_Logic::notes_get_subscribers;
          final_get_actions [i] = true;
          i = Sync_Logic::notes_get_assignees;
          final_get_actions [i] = true;
          i = Sync_Logic::notes_get_modified;
          final_get_actions [i] = true;
          break;
        }
        case Sync_Logic::notes_put_subscribe: break;
        case Sync_Logic::notes_put_unsubscribe: break;
        case Sync_Logic::notes_put_assign: break;
        case Sync_Logic::notes_put_unassign: break;
        case Sync_Logic::notes_put_status: break;
        case Sync_Logic::notes_put_passages: break;
        case Sync_Logic::notes_put_severity: break;
        case Sync_Logic::notes_put_bible: break;
        case Sync_Logic::notes_put_mark_delete: break;
        case Sync_Logic::notes_put_unmark_delete: break;
        case Sync_Logic::notes_put_delete: break;
      }
    }
    
    
    // Deal with the extra, added, note actions.
    for (int action = Sync_Logic::notes_get_total; action <= Sync_Logic::notes_get_modified; action++) {
      map <string, string> post;
      post ["u"] = bin2hex (user);
      post ["i"] = convert_to_string (identifier);
      post ["a"] = convert_to_string (action);
      response = sync_logic.post (post, url, error);
      if (error.empty ()) {
        if (action == Sync_Logic::notes_get_contents) {
          if (response != database_notes.getContents (identifier)) {
            database_notes.setContents (identifier, response);
          }
        }
        if (action == Sync_Logic::notes_get_subscribers) {
          vector <string> subscribers = filter_string_explode (response, '\n');
          database_notes.setSubscribers (identifier, subscribers);
        }
        if (action == Sync_Logic::notes_get_assignees) {
          vector <string> assignees = filter_string_explode (response, '\n');
          database_notes.setAssignees (identifier, assignees);
        }
        if (action == Sync_Logic::notes_get_modified) {
          database_notes.setModified (identifier, convert_to_int (response));
        }
      }
    }
  }
  
  
  // After all note actions have been sent to the server, and the notes updated on the client,
  // the client will now sync its notes with the server's notes.
  if (!tasks_logic_queued (DOWNLOADNOTES)) {
    int lowId = Notes_Logic::lowNoteIdentifier;
    int highId = Notes_Logic::highNoteIdentifier;
    tasks_logic_queue (DOWNLOADNOTES, { convert_to_string (lowId), convert_to_string (highId) });
  }
  sendreceive_notes_done ();
}


// The function is called with the first and last note identifier to deal with.
void sendreceive_notes_download (int lowId, int highId)
{
  Webserver_Request request;
  Database_Notes database_notes = Database_Notes (&request);
  Sync_Logic sync_logic = Sync_Logic (&request);
  Notes_Logic notes_logic = Notes_Logic (&request);
  
  
  string response = client_logic_connection_setup ();
  int iresponse = convert_to_int (response);
  if (iresponse < Filter_Roles::guest () || iresponse > Filter_Roles::admin ()) {
    Database_Logs::log (sendreceive_notes_text () + translate("Failure to initiate connection"), Filter_Roles::translator ());
    return;
  }
  
  
  // The client is supposed to have one user.
  // Get this username, plus the md5 hash of the password,
  // in preparation for sending it to the server in in the script below.
  // The server will use this user to find out the Bibles this user has access to,
  // so the server can select the correct notes for this user.
  // The client selects all available notes on the system.
  vector <string> users = request.database_users ()->getUsers ();
  if (users.empty ()) {
    Database_Logs::log (sendreceive_notes_text () + translate("No local user found"), Filter_Roles::translator ());
    return;
  }
  string user = users [0];
  request.session_logic ()->setUsername (user);
  string password = request.database_users ()->getmd5 (user);

  
  // Check for the health of the notes databases and take action if needed.
  bool healthy = true;
  if (!database_notes.healthy ()) healthy = false;
  if (!database_notes.checksums_healthy ()) healthy = false;
  if (!healthy) {
    Database_Logs::log (sendreceive_notes_text () + "Abort receive just now because of database problems", Filter_Roles::translator ());
    return;
  }
  
  
  // Server URL to call.
  string address = Database_Config_General::getServerAddress ();
  int port = Database_Config_General::getServerPort ();
  string url = client_logic_url (address, port, sync_notes_url ());
  

  // Whether this is the main script.
  bool main_script = (lowId == Notes_Logic::lowNoteIdentifier) && (highId == Notes_Logic::highNoteIdentifier);
  
  
  // The basic request to be POSTed to the server.
  map <string, string> post;
  post ["u"] = bin2hex (user);
  post ["l"] = convert_to_string (lowId);
  post ["h"] = convert_to_string (highId);

  
  // Error variable.
  string error;
  
  
  // The script requests the total note count from the server, and their total checksum.
  // It compares this with the local notes.
  // If it matches, that means that the local notes match the notes on the server.
  // The script is then ready.
  post ["a"] = convert_to_string (Sync_Logic::notes_get_total);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log (sendreceive_notes_text () + "Failure requesting totals: " + error, Filter_Roles::translator ());
    return;
  }
  vector <string> vresponse = filter_string_explode (response, '\n');
  int server_total = 0;
  if (vresponse.size () >= 1) server_total = convert_to_int (vresponse [0]);
  string server_checksum;
  if (vresponse.size () >= 2) server_checksum = vresponse [1];
  vector <int> identifiers = database_notes.getNotesInRangeForBibles (lowId, highId, {}, true);
  int client_total = identifiers.size ();
  string client_checksum = database_notes.getMultipleChecksum (identifiers);
  if (server_total == client_total) {
    if (server_checksum == client_checksum) {
      if (main_script) Database_Logs::log (sendreceive_notes_up_to_date_text (), Filter_Roles::translator ());
      return;
    }
  }
  
  
  // At this stage the total note count and/or their checksum on the client differs
  // from the total note count and/or their checksum on the server.
  // This applies to a certain range of notes for certain Bibles.


  // We know the total number of notes.
  // If the total note count is too high, divide the range of notes into smaller ranges,
  // and then schedule a task for each range.
  if (server_total > 20) {
     vector <Sync_Logic_Range> ranges = sync_logic.create_range (lowId, highId);
    for (auto range : ranges) {
      tasks_logic_queue (DOWNLOADNOTES, {to_string (range.low), convert_to_string (range.high)});
    }
    return;
  }
  
  
  // At this stage the total note count is small enough that the function can
  // start to deal with the individual notes.
  // It requests the note identifiers, their checksums,
  // and compares them with the local notes.
  // If needed, it downloads the notes from the server.
  // If needed, the client removes local notes no longer available on the server.
  
  
  // Get note identifiers and checksums as on the server.
  post ["a"] = convert_to_string (Sync_Logic::notes_get_identifiers);
  response = sync_logic.post (post, url, error);
  if (!error.empty ()) {
    Database_Logs::log (sendreceive_notes_text () + "Failure requesting identifiers: " + error, Filter_Roles::translator ());
    return;
  }
  vector <int> server_identifiers;
  vector <string> server_checksums;
  vresponse = filter_string_explode (response, '\n');
  for (size_t i = 0; i < vresponse.size (); i++) {
    if (i % 2 == 0) {
      int identifier = convert_to_int (vresponse [i]);
      if (identifier > 0) server_identifiers.push_back (identifier);
    }
    else server_checksums.push_back (vresponse [i]);
  }
  

  // Get note identifiers as locally on the client.
  vector <int> client_identifiers = database_notes.getNotesInRangeForBibles (lowId, highId, {}, true);
  
  
  // The client deletes notes no longer on the server.
  identifiers = filter_string_array_diff (client_identifiers, server_identifiers);
  for (auto identifier : identifiers) {
    string summary = database_notes.getSummary (identifier);
    database_notes.erase (identifier);
    Database_Logs::log (sendreceive_notes_text () + "Deleting one because it is not on the server: " + summary, Filter_Roles::translator ());
  }
  

  // Check whether the local notes on the client match the ones on the server.
  // If needed download the note from the server.
  for (size_t i = 0; i < server_identifiers.size (); i++) {

    if (i >= server_checksums.size ()) continue;
    
    int identifier = server_identifiers [i];
    
    string server_checksum = server_checksums [i];
    string client_checksum = database_notes.getChecksum (identifier);
    if (client_checksum == server_checksum) continue;
    
    // If the note does not yet exist locally, create a dummy note and set the correct identifier.
    if (!database_notes.identifierExists (identifier)) {
      int id = database_notes.storeNewNote ("", 0, 0, 0, "dummy", "dummy", true);
      database_notes.setIdentifier (id, identifier);
    }

    // Fetch the note from the server.
    map <string, string> post;
    post ["i"] = convert_to_string (identifier);

    post ["a"] = convert_to_string (Sync_Logic::notes_get_summary);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting summary: " + error, Filter_Roles::translator ());
      return;
    }
    string summary = response;
    if (summary != database_notes.getSummary (identifier)) {
      database_notes.setSummary (identifier, summary);
    }
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_contents);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting contents: " + error, Filter_Roles::translator ());
      return;
    }
    if (response != database_notes.getContents (identifier)) {
      database_notes.setContents (identifier, response);
    }
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_subscribers);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting subscribers: " + error, Filter_Roles::translator ());
      return;
    }
    vector <string> subscribers = filter_string_explode (response, '\n');
    database_notes.setSubscribers (identifier, subscribers);
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_assignees);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting assignees: " + error, Filter_Roles::translator ());
      return;
    }
    vector <string> assignees = filter_string_explode (response, '\n');
    database_notes.setAssignees (identifier, assignees);
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_status);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting status: " + error, Filter_Roles::translator ());
      return;
    }
    database_notes.setStatus (identifier, response);
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_passages);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting passages: " + error, Filter_Roles::translator ());
      return;
    }
    vector <string> lines = filter_string_explode (response, '\n');
    vector <Passage> passages;
    for (auto & line : lines) {
      passages.push_back (filter_integer_to_passage (convert_to_int (line)));
    }
    database_notes.setPassages (identifier, passages);
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_severity);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting severity: " + error, Filter_Roles::translator ());
      return;
    }
    database_notes.setRawSeverity (identifier, convert_to_int (response));
    
    post ["a"] = convert_to_string (Sync_Logic::notes_get_bible);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting bible: " + error, Filter_Roles::translator ());
      return;
    }
    database_notes.setBible (identifier, response);
    
    // Set time modified field last as it is affected by the previous store actions.
    post ["a"] = convert_to_string (Sync_Logic::notes_get_modified);
    response = sync_logic.post (post, url, error);
    if (!error.empty ()) {
      Database_Logs::log (sendreceive_notes_text () + "Failure requesting time modified: " + error, Filter_Roles::translator ());
      return;
    }
    database_notes.setModified (identifier, convert_to_int (response));
    
    // Update search and checksum.
    database_notes.updateSearchFields (identifier);
    database_notes.updateChecksum (identifier);

    Database_Logs::log (sendreceive_notes_text () + "Received from server: " + summary, Filter_Roles::manager ());

    database_notes.getChecksum (identifier);
  }
}

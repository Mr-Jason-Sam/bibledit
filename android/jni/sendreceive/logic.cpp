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


#include <sendreceive/logic.h>
#include <filter/url.h>
#include <filter/date.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/bibles.h>
#include <client/logic.h>


void sendreceive_queue_bible (string bible)
{
  tasks_logic_queue (SENDRECEIVEBIBLES, {bible});
}


// If $minute is negative, it syncs.
// If $minute >=0, it determines from the settings whether to sync.
void sendreceive_queue_sync (int minute)
{
  // Deal with a numerical minute to find out whether it's time to automatically sync.
  if (minute >= 0) {
    int repeat = Database_Config_General::getRepeatSendReceive ();
    // Sync every hour.
    if (repeat == 1) {
      minute = minute % 60;
      if (minute == 0) minute = -1;
    }
    // Sync every five minutes.
    if (repeat == 2) {
      minute = minute % 5;
      if (minute == 0) minute = -1;
    }
  }
  
  // Send and receive: It is time now, or it is manual.
  // Only queue a sync task if it is not running at the moment.
  if (minute < 0) {
    
    // Send / receive only works in Client mode.
    if (client_logic_client_enabled ()) {
      
      if (tasks_logic_queued (SYNCNOTES)) {
        Database_Logs::log ("Still synchronizing notes");
      } else {
        tasks_logic_queue (SYNCNOTES);
      }
      if (tasks_logic_queued (SYNCBIBLES)) {
        Database_Logs::log ("Still synchronizing Bibles");
      } else {
        tasks_logic_queue (SYNCBIBLES);
      }
      if (tasks_logic_queued (SYNCSETTINGS)) {
        Database_Logs::log ("Still synchronizing settings");
      } else {
        tasks_logic_queue (SYNCSETTINGS);
      }
      if (tasks_logic_queued (SYNCCHANGES)) {
        Database_Logs::log ("Still synchronizing Changes");
      } else {
        tasks_logic_queue (SYNCCHANGES);
      }
      if (tasks_logic_queued (SYNCFILES)) {
        Database_Logs::log ("Still synchronizing Files");
      } else {
        tasks_logic_queue (SYNCFILES);
      }
    }
    
    // Paratext collaboration.
    if (config_logic_paratext_enabled ()) {
      if (tasks_logic_queued (SYNCPARATEXT)) {
        Database_Logs::log ("Still synchronizing with Paratext");
      } else {
        tasks_logic_queue (SYNCPARATEXT);
      }
    }
    // Store the most recent time that the sync action ran.
    Database_Config_General::setLastSendReceive (filter_date_seconds_since_epoch ());
  }
}


// Checks whether any of the sync tasks has been queued.
// Returns the result as a boolean.
bool sendreceive_sync_queued ()
{
  if (tasks_logic_queued (SYNCNOTES)) return true;
  if (tasks_logic_queued (SYNCBIBLES)) return true;
  if (tasks_logic_queued (SYNCSETTINGS)) return true;
  if (tasks_logic_queued (SYNCCHANGES)) return true;
  if (tasks_logic_queued (SYNCFILES)) return true;
  return false;
}


void sendreceive_queue_all (bool now)
{
  Database_Bibles database_bibles = Database_Bibles ();
  vector <string> bibles = database_bibles.getBibles ();
  for (auto & bible : bibles) {
    if (Database_Config_Bible::getRemoteRepositoryUrl (bible) != "") {
      if (Database_Config_Bible::getRepeatSendReceive (bible) || now) {
        sendreceive_queue_bible (bible);
      }
    }
  }
}


// Function that looks if, at app startup, it needs to queue sync operations in the client.
void sendreceive_queue_startup ()
{
  // Next second when it is supposed to sync.
  int next_second = Database_Config_General::getLastSendReceive ();

  // Check how often to repeat the sync action.
  int repeat = Database_Config_General::getRepeatSendReceive ();
  if (repeat == 1) {
    // Repeat every hour.
    next_second += 3600;
  }
  else if (repeat == 2) {
    // Repeat every five minutes.
    next_second += 300;
  } else {
    // No repetition.
    return;
  }
  
  // When the current time is past the next time it is supposed to sync, start the sync.
  if (filter_date_seconds_since_epoch () >= next_second) {
    sendreceive_queue_sync (-1);
  }
}

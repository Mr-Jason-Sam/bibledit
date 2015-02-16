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


#include <tasks/run.h>
#include <tasks/logic.h>
#include <database/logs.h>
#include <filter/string.h>
#include <filter/url.h>
#include <mutex>
#include <email/receive.h>
#include <email/send.h>
#include <search/rebibles.h>
#include <search/renotes.h>
#include <styles/sheets.h>
#include <bible/import_task.h>
#include <compare/compare.h>
#include <database/maintenance.h>
#include <tmp/tmp.h>
#include <collaboration/link.h>
#include <sendreceive/sendreceive.h>
#include <sendreceive/settings.h>
#include <sendreceive/bibles.h>
#include <demo/logic.h>


mutex mutex_tasks; 
int running_tasks = 0;


void tasks_run_one (string filename)
{
  // Increase running tasks count.
  mutex_tasks.lock ();
  running_tasks++;
  mutex_tasks.unlock ();

  // Read the task from disk and erase the file.
  string path = filter_url_create_path (tasks_logic_folder (), filename);
  vector <string> lines = filter_string_explode (filter_url_file_get_contents (path), '\n');
  filter_url_unlink (path);
  
  // Interpret the task's command and its parameters, if any.
  string command = "";
  if (!lines.empty ()) {
    command = lines [0];
    lines.erase (lines.begin ());
  }
  string parameter1;
  if (!lines.empty ()) {
    parameter1 = lines [0];
    lines.erase (lines.begin ());
  }
  string parameter2;
  if (!lines.empty ()) {
    parameter2 = lines [0];
    lines.erase (lines.begin ());
  }
  string parameter3;
  if (!lines.empty ()) {
    parameter3 = lines [0];
    lines.erase (lines.begin ());
  }
  
  if (command == ROTATEJOURNAL) {
    Database_Logs database_logs = Database_Logs ();
    database_logs.checkup ();
    database_logs.rotate ();
  } else if (command == RECEIVEEMAIL) {
    email_receive ();
  } else if (command == SENDEMAIL) {
    email_send ();
  } else if (command == REINDEXBIBLES) {
    search_reindex_bibles ();
  } else if (command == REINDEXNOTES) {
    search_reindex_notes ();
  } else if (command == CREATECSS) {
    styles_sheets_create_all_run ();
  } else if (command == IMPORTUSFM) {
    bible_import_task (parameter1, parameter2);
  } else if (command == COMPAREUSFM) {
    compare_compare (parameter1, parameter2, convert_to_int (parameter3));
  } else if (command == MAINTAINDATABASE) {
    database_maintenance ();
  } else if (command == CLEANTMPFILES) {
    tmp_tmp ();
  } else if (command == LINKGITREPOSITORY) {
    collaboration_link (parameter1, convert_to_int (parameter2), parameter3);
  } else if (command == SENDRECEIVEBIBLES) {
    sendreceive_sendreceive (parameter1);
  } else if (command == SYNCNOTES) {
    Database_Logs::log ("Not yet implemented: " + command); // syncnotes.php
  } else if (command == SYNCBIBLES) {
    sendreceive_bibles ();
  } else if (command == SYNCSETTINGS) {
    sendreceive_settings ();
  } else if (command == SYNCEXTERNALRESOURCES) {
    Database_Logs::log ("Not yet implemented: " + command); // externalresources.php
  } else if (command == SYNCUSFMRESOURCES) {
    Database_Logs::log ("Not yet implemented: " + command); // usfmresources.php
  } else if (command == CLEANDEMO) {
    demo_clean_data ();
  } else {
    Database_Logs::log ("Unknown task: " + command);
  }
  

  // Decrease running tasks count.
  mutex_tasks.lock ();
  running_tasks--;
  mutex_tasks.unlock ();
}


void tasks_run_check ()
{
  // Don't run more than so many tasks.
  mutex_tasks.lock ();
  int taskscount = running_tasks;
  mutex_tasks.unlock ();
  if (taskscount >= 10) return;
  // Get and start first available task.
  vector <string> tasks = filter_url_scandir (tasks_logic_folder ());
  if (tasks.empty ()) return;
  thread task_thread = thread (tasks_run_one, tasks [0]);
  // Detach the thread to the thread continues to run independently,
  // when the thread object goes out of scope, and no memory is leaked this way.
  task_thread.detach ();
}

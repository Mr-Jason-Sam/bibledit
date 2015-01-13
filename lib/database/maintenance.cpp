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


#include <database/maintenance.h>
#include <filter/roles.h>
#include <filter/url.h>
#include <config/logic.h>
#include <webserver/request.h>
#include <database/logs.h>
#include <database/users.h>
#include <database/mail.h>
#include <database/confirm.h>
#include <database/books.h>
#include <database/bibles.h>
#include <database/styles.h>
#include <database/ipc.h>
#include <database/notes.h>
#include <database/check.h>
#include <database/volatile.h>
#include <database/sprint.h>
#include <database/navigation.h>
#include <database/sprint.h>
#include <database/history.h>
#include <database/commits.h>
#include <database/jobs.h>
#include <database/config/user.h>



void database_maintenance ()
{
  Database_Logs::log ("Database maintenance", Filter_Roles::admin ());
  
  
  // Whether running in client mode.
  bool client_mode = config_logic_client_enabled ();
  
  
  Webserver_Request webserver_request;
  
  
  // While VACUUM or REINDEX on a SQLite database are running,
  // querying the database then introduces errors like "database schema has changed".
  // Therefore this type of maintenance should not be done automatically.
  
  
  Database_Users database_users = Database_Users ();
  database_users.trim ();
  database_users.optimize ();
  
  
  Database_Mail database_mail = Database_Mail (&webserver_request);
  database_mail.trim ();
  database_mail.optimize ();
  
  
  Database_Confirm database_confirm = Database_Confirm ();
  database_confirm.trim ();
  database_confirm.optimize ();
  
  
  // No need to optimize the following because it is hardly ever written to.
  // Database_Books database_book = Database_Books ();
  // Database_Styles database_styles = Database_Styles ();
  
  
  Database_Bibles database_bibles = Database_Bibles ();
  database_bibles.optimize ();

  
  Database_Ipc database_ipc = Database_Ipc (&webserver_request);
  database_ipc.trim ();
  
  
  Database_Notes database_notes = Database_Notes (&webserver_request);
  database_notes.trim ();
  if (!client_mode) database_notes.trim_server ();
  database_notes.optimize ();
  
  
  /*
  Database_Shell database_shell = Database_Shell ();
  database_shell.trim ();
  database_shell.optimize ();
  */
  
  
  Database_Check database_check = Database_Check ();
  database_check.optimize ();
  
  
  filter_url_unlink (filter_url_create_root_path ("databases", "volatile.sqlite"));
  Database_Volatile database_volatile = Database_Volatile ();
  database_volatile.create ();
  
  
  Database_Sprint database_sprint = Database_Sprint ();
  database_sprint.optimize ();
  
  
  filter_url_unlink (filter_url_create_root_path ("databases", "navigation.sqlite"));
  Database_Navigation database_navigation = Database_Navigation ();
  database_navigation.create ();
  
  
  Database_History database_history = Database_History ();
  database_history.optimize ();
  
  
  Database_Commits database_commits = Database_Commits ();
  database_commits.optimize ();
  
  
  Database_Jobs database_jobs = Database_Jobs ();
  database_jobs.trim ();
  database_jobs.optimize ();
  
  
  Database_Config_User database_config_user = Database_Config_User (&webserver_request);
  database_config_user.trim ();
}
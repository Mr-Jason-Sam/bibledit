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


#include <notes/poll.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/header.h>
#include <filter/roles.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <database/notes.h>
// Todo #include <ipc/notes.h>


string notes_poll_url ()
{
  return "notes/poll";
}


bool notes_poll_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::consultant ());
}


string notes_poll (void * webserver_request) // Todo
{
  /*
  Ipc_Notes ipc_notes = Ipc_Notes (webserver_request);
  $action = request->query ['action'];
  if ($action == "alive") {
    $ipc_notes->alive (true);
    $identifier = $ipc_notes->get ();
    if ($identifier) {
      $ipc_notes->delete ();
      $url = "note?id=$identifier";
      echo $url;
    }
  } else if ($action == "unload") {
    $ipc_notes->alive (false);
  }
  return "";
  */
}

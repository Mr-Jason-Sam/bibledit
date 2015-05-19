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


#include <debug/index.h>
#include <assets/view.h>
#include <assets/page.h>
#include <filter/roles.h>
#include <locale/translate.h>
#include <database/notes.h>


const char * debug_index_url ()
{
  return "debug/index";
}


bool debug_index_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::admin ());
}


string debug_index (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  string page;

  page = Assets_Page::header ("Bibledit", webserver_request, "");

  Assets_View view = Assets_View ();

  if (request->query.count ("debug")) {
    /*
    Database_Notes database_notes = Database_Notes (&webserver_request);
    database_notes.trim ();
    database_notes.trim_server ();
    database_notes.optimize ();
    view.set_variable ("success", translate ("Command activated"));
    */
  }
  
  page += view.render ("debug", "index");
  page += Assets_Page::footer ();

  return page;
}
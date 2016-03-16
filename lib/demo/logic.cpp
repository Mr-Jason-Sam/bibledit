/*
 Copyright (©) 2003-2016 Teus Benschop.
 
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


#include <demo/logic.h>
#include <filter/md5.h>
#include <filter/roles.h>
#include <filter/usfm.h>
#include <filter/url.h>
#include <filter/string.h>
#include <config/logic.h>
#include <config.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/notes.h>
#include <locale/translate.h>
#include <client/logic.h>
#include <styles/logic.h>
#include <styles/sheets.h>
#include <locale/logic.h>
#include <bible/logic.h>
#include <edit/index.h>
#include <editusfm/index.h>
#include <resource/index.h>
#include <resource/external.h>
#include <resource/logic.h>
#include <workbench/logic.h>
#include <ipc/focus.h>
#include <lexicon/logic.h>
#include <search/logic.h>


/*

 A demo installation is an open installation.
 A user is always considered to be logged in as admin.
 
 In October 2015 the demo began to often refuse web connections.
 It appears that the server keeps running most of the times, but also crashed often during certain periods.

 The number of parallel connections was traced to see if that was the cause. 
 The parallel connection count was mostly 0, at times 1, and higher at rare occassions.
 So this should be excluded as the cause.
 
 Continuous crashes of the server are the likely cause.
 The page requests are now being logged to see what happens.
 After logging them, it appears that the crash often comes after /resource/get

 Next a crash handler was installed, which gives some sort of backtrace in the Journal.
 This showed one crash during the nights. The crash was fixed.
 
*/

// Returns true for correct credentials for a demo installation.
// Else returns false.
bool demo_acl (string user, string pass)
{
  if (strcmp (DEMO, "yes") == 0) {
    if (user == session_admin_credentials ()) {
      if ((pass == session_admin_credentials ()) || (pass == md5 (session_admin_credentials ()))) {
        return true;
      }
    }
  }
  return false;
}


// Returns the address of the current demo server.
string demo_address ()
{
  return "http://bibledit.org";
}


int demo_port ()
{
  return 8080;
}


// Returns a warning in case the client is connected to the open demo server.
string demo_client_warning ()
{
  string warning;
  if (client_logic_client_enabled ()) {
    string address = Database_Config_General::getServerAddress ();
    if (address == demo_address ()) {
      int port = Database_Config_General::getServerPort ();
      if (port == demo_port ()) {
        warning.append (translate("You are connected to a public demo of Bibledit Cloud."));
        warning.append (" ");
        warning.append (translate("Everybody can modify the data on that server."));
        warning.append (" ");
        warning.append (translate("After send and receive your data will reflect the data on the server."));
      }
    }
  }
  return warning;
}


// Cleans and resets the data in the Bibledit installation.
void demo_clean_data ()
{
  Database_Logs::log ("Cleaning up the demo data");

  
  Webserver_Request request;
  
  
  // Set user to the demo credentials (admin) as this is the user who is always logged-in in a demo installation.
  request.session_logic ()->setUsername (session_admin_credentials ());
  
  
  // Delete empty stylesheet that may have been there.
  request.database_styles()->revokeWriteAccess ("", styles_logic_standard_sheet ());
  request.database_styles()->deleteSheet ("");
  styles_sheets_create_all ();
  
  
  // Set the export stylesheet to "Standard" for all Bibles and the admin.
  vector <string> bibles = request.database_bibles()->getBibles ();
  for (auto & bible : bibles) {
    Database_Config_Bible::setExportStylesheet (bible, styles_logic_standard_sheet ());
  }
  request.database_config_user()->setStylesheet (styles_logic_standard_sheet ());
  
  
  // Set the site language to "Default"
  Database_Config_General::setSiteLanguage ("");
  
  
  // Ensure the default users are there.
  map <string, int> users = {
    make_pair ("guest", Filter_Roles::guest ()),
    make_pair ("member", Filter_Roles::member ()),
    make_pair ("consultant", Filter_Roles::consultant ()),
    make_pair ("translator", Filter_Roles::translator ()),
    make_pair ("manager", Filter_Roles::manager ()),
    make_pair (session_admin_credentials (), Filter_Roles::admin ())
  };
  for (auto & element : users) {
    if (!request.database_users ()->usernameExists (element.first)) {
      request.database_users ()->addNewUser(element.first, element.first, element.second, "");
    }
    request.database_users ()->updateUserLevel (element.first, element.second);
  }

  
  // Create / update sample Bible.
  demo_create_sample_bible ();


  // Clean out nearly empty chapters from the Bibles.
  bibles = request.database_bibles()->getBibles ();
  for (auto bible : bibles) {
    vector <int> books = request.database_bibles()->getBooks (bible);
    for (auto book : books) {
      vector <int> chapters = request.database_bibles()->getChapters (bible, book);
      for (auto chapter : chapters) {
        // Remove chapters, other than 0, that are rather short, as these chapters likely contain no text, but USFM markers only.
        if (chapter == 0) continue;
        string usfm = request.database_bibles()->getChapter (bible, book, chapter);
        if (usfm.length () < 200) {
          Database_Logs::log ("Deleting a demo chapter because it does not contain enough text: " + convert_to_string (book) + ":" + convert_to_string (chapter));
          Bible_Logic::deleteChapter (bible, book, chapter);
        }
      }
      // If a book contains chapter 0 only, remove that entire book.
      chapters = request.database_bibles()->getChapters (bible, book);
      if ((chapters.size () == 1) && (chapters [0] == 0)) {
        Database_Logs::log ("Deleting a demo book because it is empty");
        Bible_Logic::deleteBook (bible, book);
      }
    }
    // If a Bible contains no books, remove that Bible.
    books = request.database_bibles()->getBooks (bible);
    if (books.empty ()) {
      Database_Logs::log ("Deleting a demo Bible because it is empty");
      Bible_Logic::deleteBible (bible);
    }
  }
  
  
  // Create sample notes.
  demo_create_sample_notes (&request);
  
  
  // Create samples for the workbenches.
  demo_create_sample_workbenches (&request);
  
  
  // Set navigator to John 3:16.
  Ipc_Focus::set (&request, 43, 3, 16);
  
  
  // Set and/or trim resources to display.
  // Too many resources crash the demo, so limit the amount.
  vector <string> resources = request.database_config_user()->getActiveResources ();
  bool reset_resources = false;
  if (resources.size () > 25) reset_resources = true;
  vector <string> defaults = demo_logic_default_resources ();
  for (auto & name : defaults) {
    if (!in_array (name, resources)) reset_resources = true;
  }
  if (reset_resources) {
    resources = demo_logic_default_resources ();
    request.database_config_user()->setActiveResources (resources);
  }
  
  
  // No flipped basic <> advanded mode.
  request.database_config_user ()->setFlipInterfaceMode (false);
}


// The name of the sample Bible.
string demo_sample_bible_name ()
{
  return "Bibledit Sample Bible";
}


// Creates a sample Bible.
void demo_create_sample_bible () // Todo this is going to copy the bible rather than creating it.
{
  // Ensure the sample Bible exists.
  Database_Bibles database_bibles;
  database_bibles.createBible (demo_sample_bible_name ());
  
  // Store some text into the sample Bible.
  string directory = filter_url_create_root_path ("demo");
  vector <string> files = filter_url_scandir (directory);
  for (auto file : files) {
    if (filter_url_get_extension (file) == "usfm") {
      Database_Logs::log ("Creating sample Bible book: " + file);
      file = filter_url_create_path (directory, file);
      string usfm = filter_url_file_get_contents (file);
      usfm = filter_string_str_replace ("  ", " ", usfm);
      vector <BookChapterData> book_chapter_data = usfm_import (usfm, styles_logic_standard_sheet ());
      for (auto data : book_chapter_data) {
        Bible_Logic::storeChapter (demo_sample_bible_name (), data.book, data.chapter, data.data);
      }
    }
  }
  
  Database_Logs::log ("Ready creating sample Bible");
}


// Prepares a sample Bible.
// The output of this is supposed to be manually put into the source tree, folder "samples".
// This will be used to quickly create a sample Bible, that is fast, even on mobile devices.
void demo_prepare_sample_bible ()
{
  Database_Bibles database_bibles;
  // Remove the Bible to remove all stuff that might have been in it.
  database_bibles.deleteBible (demo_sample_bible_name ());
  search_logic_delete_bible (demo_sample_bible_name ());
  // Create a new one.
  database_bibles.createBible (demo_sample_bible_name ());
  // Location of the USFM files for the sample Bible.
  string directory = filter_url_create_root_path ("demo");
  vector <string> files = filter_url_scandir (directory);
  for (auto file : files) {
    // Only process the USFM files.
    if (filter_url_get_extension (file) == "usfm") {
      cout << file << endl;
      // Read the USFM.
      file = filter_url_create_path (directory, file);
      string usfm = filter_url_file_get_contents (file);
      usfm = filter_string_str_replace ("  ", " ", usfm);
      // Import the USFM into the Bible.
      vector <BookChapterData> book_chapter_data = usfm_import (usfm, styles_logic_standard_sheet ());
      for (auto data : book_chapter_data) {
        Bible_Logic::storeChapter (demo_sample_bible_name (), data.book, data.chapter, data.data);
      }
    }
  }
  // Clean the destination location for the Bible.
  string destination = sample_bible_bible_path ();
  filter_url_rmdir (destination);
  // Copy the Bible data to the destination.
  string source = database_bibles.bibleFolder (demo_sample_bible_name ());
  filter_url_dir_cp (source, destination);
  // Clean the destination location for the Bible search index.
  destination = sample_bible_index_path ();
  filter_url_rmdir (destination);
  // Create destination location.
  filter_url_mkdir (destination);
  // Copy the index files over to the destination.
  source = search_logic_index_folder ();
  files = filter_url_scandir (source);
  for (auto file : files) {
    if (file.find (demo_sample_bible_name ()) != string::npos) {
      string source_file = filter_url_create_path (source, file);
      string destination_file = filter_url_create_path (destination, file);
      filter_url_file_cp (source_file, destination_file);
    }
  }
}


// Create sample notes.
void demo_create_sample_notes (void * webserver_request)
{
  Database_Notes database_notes (webserver_request);
  vector <int> identifiers = database_notes.getIdentifiers ();
  if (identifiers.size () < 10) {
    for (size_t i = 1; i <= 10; i++) {
      database_notes.storeNewNote (demo_sample_bible_name (), i, i, i, "Sample Note " + convert_to_string (i), "Sample Contents for note " + convert_to_string (i), false);
    }
  }
}


string demo_workbench ()
{
  return "Translation";
}


void demo_create_sample_workbenches (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  map <int, string> urls;
  map <int, string> widths;
  for (int i = 0; i < 15; i++) {
    string url;
    string width;
    if (i == 0) {
      url = editusfm_index_url ();
      width = "45%";
    }
    if (i == 1) {
      url = resource_index_url ();
      width = "45%";
    }
    urls [i] = url;
    widths [i] = width;
  }
  map <int, string> row_heights = {
    make_pair (0, "90%"),
    make_pair (1, ""),
    make_pair (2, "")
  };

  request->database_config_user()->setActiveWorkbench ("USFM");
  workbench_set_urls (request, urls);
  workbench_set_widths (request, widths);
  workbench_set_heights (request, row_heights);

  urls[0] = edit_index_url ();
  urls[1] = resource_index_url ();

  request->database_config_user()->setActiveWorkbench (demo_workbench ());
  workbench_set_urls (request, urls);
  workbench_set_widths (request, widths);
  workbench_set_heights (request, row_heights);
}


vector <string> demo_logic_default_resources ()
{
  return {
    demo_sample_bible_name (),
    resource_logic_violet_divider (),
    resource_external_biblehub_interlinear_name (),
    resource_external_net_bible_name (),
    SBLGNT_NAME
  };
}


string sample_bible_bible_path ()
{
  return filter_url_create_root_path ("samples", "bible");
}


string sample_bible_index_path ()
{
  return filter_url_create_root_path ("samples", "index");
}

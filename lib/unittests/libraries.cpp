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


#include <unittests/libraries.h>
#include <unittests/utilities.h>
#include <filter/string.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/md5.h>
#include <filter/usfm.h>
#include <flate/flate.h>
#include <config/globals.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/sqlite.h>
#include <database/users.h>
#include <database/books.h>
#include <checksum/logic.h>
#include <editor/export.h>
#include <editor/import.h>


void test_sqlite ()
{
  // Tests for SQLite.
  sqlite3 * db = database_sqlite_connect ("sqlite");
  if (!db) error_message (__LINE__, __func__, "pointer", "NULL");
  database_sqlite_exec (db, "CREATE TABLE test (column1 integer, column2 integer, column3 integer);");
  database_sqlite_exec (db, "INSERT INTO test VALUES (123, 456, 789);");
  database_sqlite_exec (db, "INSERT INTO test VALUES (234, 567, 890);");
  database_sqlite_exec (db, "INSERT INTO test VALUES (345, 678, 901);");
  map <string, vector <string> > actual = database_sqlite_query (db, "SELECT column1, column2, column3 FROM test;");
  evaluate (__LINE__, __func__, "567", actual ["column2"] [1]);
  database_sqlite_disconnect (db);
  database_sqlite_disconnect (NULL);

  evaluate (__LINE__, __func__, true, database_sqlite_healthy ("sqlite"));
  unlink (database_sqlite_file ("sqlite").c_str());
  evaluate (__LINE__, __func__, false, database_sqlite_healthy ("sqlite"));

  evaluate (__LINE__, __func__, "He''s", database_sqlite_no_sql_injection ("He's"));
}


void test_session_logic ()
{
  // Test for class Session_Logic.
  {
    refresh_sandbox (true);
    
    // The session logic depends on users in the database.
    Database_Users database_users = Database_Users ();
    database_users.create ();
    
    // In an open installation, a client is always logged in as user admin, even after logging out.
    config_globals_open_installation = true;
    Webserver_Request request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, "admin", request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::admin (), request.session_logic ()->currentLevel ());
    request.session_logic ()->logout ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, "admin", request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::admin (), request.session_logic ()->currentLevel ());
    
    // Test function to set the username.
    string username = "ঃইঝম";
    request.session_logic ()->setUsername (username);
    evaluate (__LINE__, __func__, username, request.session_logic ()->currentUser ());
    config_globals_open_installation = false;
  }
  {
    // In a client installation, a client is logged in as admin when there's no user in the database.
    refresh_sandbox (true);
    Database_Users database_users = Database_Users ();
    database_users.create ();
    config_globals_client_prepared = true;
    Webserver_Request request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, "admin", request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::admin (), request.session_logic ()->currentLevel ());
    config_globals_client_prepared = false;
  }
  {
    // In a client installation, a client is logged in as the first user in the database.
    refresh_sandbox (true);
    Database_Users database_users = Database_Users ();
    database_users.create ();
    string username = "ঃইঝম";
    int level = 10;
    database_users.addNewUser (username, "password", level, "email");
    config_globals_client_prepared = true;
    Webserver_Request request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, username, request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, level, request.session_logic ()->currentLevel ());
    config_globals_client_prepared = false;
  }
  {
    refresh_sandbox (true);
    Database_Users database_users = Database_Users ();
    database_users.create ();
    Webserver_Request request = Webserver_Request ();

    // Enter a user into the database.
    string username = "ঃইঝম";
    string password = "ᨃᨄᨔᨕ";
    string email = "email@website";
    int level = 10;
    database_users.addNewUser (username, password, level, email);

    // Log in by providing username and password.
    evaluate (__LINE__, __func__, false, request.session_logic ()->attemptLogin (username, "incorrect"));
    evaluate (__LINE__, __func__, true, request.session_logic ()->attemptLogin (username, password));

    // Check whether logged in also from another session.
    request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, username, request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, level, request.session_logic ()->currentLevel ());
    
    // Logout in another session, and check it in a subsequent session.
    request = Webserver_Request ();
    request.session_logic ()->logout ();
    request = Webserver_Request ();
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, "", request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::guest(), request.session_logic ()->currentLevel ());
    
    // Login. then vary the browser's signature for subsequent sessions.
    request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->attemptLogin (username, password));
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    string remote_address = request.remote_address;
    string user_agent = request.user_agent;
    string accept_language = request.accept_language;
    request = Webserver_Request ();
    request.remote_address = "1.2.3.4";
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.remote_address = remote_address;
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.user_agent = "User's Agent";
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.user_agent = user_agent;
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.accept_language = "xy_ZA";
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.accept_language = accept_language;
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
  }
}


void test_empty_folders ()
{
  // There should be no empty folders in the library, because git does not include them.
  int result = system ("find . -type d -empty > /tmp/bibledittest.txt");
  evaluate (__LINE__, __func__, 0, result);
  string contents = filter_url_file_get_contents ("/tmp/bibledittest.txt");
  evaluate (__LINE__, __func__, "", contents);
  filter_url_unlink ("/tmp/bibledittest.txt");
}


void test_flate ()
{
  // Test for the flate2 template engine.
  string tpl = filter_url_create_root_path ("unittests", "tests", "flate1.html");
  Flate flate;
  string desired;
  string actual;

  flate = Flate ();
  desired = 
    "line 1\n"
    "\n"
    "line 6\n"
    "LocalizeOne\n"
    "LocalizeTwo";
  actual = filter_string_trim (flate.render (tpl));
  evaluate (__LINE__, __func__, desired, actual);

  flate = Flate ();
  flate.enable_zone ("one");
  flate.enable_zone ("two");
  desired = 
    "line 1\n"
    "\n"
    "line 2\n"
    "\n"
    "\n"
    "\n"
    "line 3\n"
    "\n"
    "line 4\n"
    "\n"
    "\n"
    "line 6\n"
    "LocalizeOne\n"
    "LocalizeTwo";
  actual = filter_string_trim (flate.render (tpl));
  evaluate (__LINE__, __func__, desired, actual);

  flate = Flate ();
  flate.enable_zone ("one");
  flate.enable_zone ("three");
  flate.set_variable ("three", "THREE");
  desired = 
    "line 1\n"
    "\n"
    "line 2\n"
    "\n"
    "\n"
    "\n"
    "line 4\n"
    "\n"
    "THREE\n"
    "line 5\n"
    "\n"
    "\n"
    "line 6\n"
    "LocalizeOne\n"
    "LocalizeTwo";
  actual = filter_string_trim (flate.render (tpl));
  evaluate (__LINE__, __func__, desired, actual);
}


void test_checksum_logic ()
{
  // Get1
  {
    string data = "\\v Verse 1";
    string checksum = Checksum_Logic::get (data);
    evaluate (__LINE__, __func__, "3", checksum);
  }
  // Get2
  {
    string data = "Line one\\nLine 2\\n";
    string checksum = Checksum_Logic::get (data);
    evaluate (__LINE__, __func__, "3", checksum);
  }
  // Send1
  {
    string data = "\\v Verse 1";
    string checksum = Checksum_Logic::send (data);
    string standard = "3\n" + data;
    evaluate (__LINE__, __func__, standard, checksum);
  }
  // Send2
  {
    string data = "Line one\\nLine 2\n";
    string checksum = Checksum_Logic::send (data);
    string standard = "3\n" + data;
    evaluate (__LINE__, __func__, standard, checksum);
  }
  // Setup some data.
  refresh_sandbox (true);
  Webserver_Request request;
  request.database_search()->create ();
  request.database_bibles()->storeChapter ("phpunit1", 1, 2, "data1");
  request.database_bibles()->storeChapter ("phpunit1", 1, 3, "data2");
  request.database_bibles()->storeChapter ("phpunit1", 1, 4, "data3");
  request.database_bibles()->storeChapter ("phpunit2", 2, 5, "data4");
  // GetChapter1
  {
    string checksum = Checksum_Logic::getChapter (&request, "phpunit1", 1, 2);
    evaluate (__LINE__, __func__, md5 ("data1"), checksum);
  }
  // GetChapter2
  {
    string checksum = Checksum_Logic::getChapter (&request, "phpunit2", 2, 6);
    evaluate (__LINE__, __func__, md5 (""), checksum);
  }
  // GetBook1
  {
    string checksum = Checksum_Logic::getBook (&request, "phpunit1", 1);
    evaluate (__LINE__, __func__, "2ab6425924e6cd38b2474c543c5ea602", checksum);
  }
  // GetBook2
  {
    string checksum = Checksum_Logic::getBook (&request, "phpunit3", 1);
    evaluate (__LINE__, __func__, "d41d8cd98f00b204e9800998ecf8427e", checksum);
  }
  // GetBible1
  {
    string checksum = Checksum_Logic::getBible (&request, "phpunit1");
    evaluate (__LINE__, __func__, "f9dc679a8712eb6f65b584e9688e9680", checksum);
  }
  // GetBible2
  {
    string checksum = Checksum_Logic::getBible (&request, "phpunit2");
    evaluate (__LINE__, __func__, "ee84a85bac14adb35e887c3d89bc80ab", checksum);
  }
  // GetBibles1
  {
    string checksum = Checksum_Logic::getBibles (&request, {"phpunit1", "phpunit2"});
    evaluate (__LINE__, __func__, "440b2008993816f0bc8c557b64fbdaf2", checksum);
  }
  // GetBibles2
  {
    string checksum = Checksum_Logic::getBibles (&request, {"phpunit3", "phpunit4"});
    evaluate (__LINE__, __func__, "020eb29b524d7ba672d9d48bc72db455", checksum);
  }
}


void test_store_bible_data_safely_setup (Webserver_Request * request, string usfm)
{
  refresh_sandbox (true);
  request->database_search()->create ();
  request->database_bibles()->createBible ("phpunit");
  request->database_bibles()->storeChapter ("phpunit", 1, 1, usfm);
}


void test_store_bible_data ()
{
  Webserver_Request request;
  string usfm =
  "\\c 1\n"
  "\\p\n"
  "\\v 1 Verse 1.\n"
  "\\v 2 Verse 2.\n"
  "\\v 3 Verse 3.\n"
  "\\v 4 Verse 4.\n"
  "\\v 5 Verse 5.\n";
  // SafeStoreChapterSaveOne
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 2 Verse 2.\n"
    "\\v 3 Verse 3.\n"
    "\\v 4 Verse 4.\n"
    "\\v 5 Verse 5.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, true, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, data, result);
  }
  // SafeStoreChapterSaveTwo
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 2 Verse 2.\n"
    "\\v 3 Verse 3.\n"
    "\\v 4 Verse 4.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, true, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, data, result);
  }
  // SafeStoreChapterLength
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 2 Verse 2.\n"
    "\\v 3 Verse 3.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, false, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, usfm, result);
    refresh_sandbox (false);
  }
  // SafeStoreChapterSimilarity
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 3 Verse 3.\n"
    "\\v 2 Verse 2.\n"
    "\\v 4 Verse 4.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, false, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, usfm, result);
    refresh_sandbox (false);
  }
  // SafeStoreChapterNoChange
  {
    test_store_bible_data_safely_setup (&request, usfm);
    int currentId = request.database_bibles()->getChapterId ("phpunit", 1, 1);
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, usfm);
    evaluate (__LINE__, __func__, true, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, usfm, result);
    int currentId2 = request.database_bibles()->getChapterId ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, currentId, currentId2);
  }
}


void test_editor_export () // Todo
{
  // Basic test.
  {
    Webserver_Request request;
    string html = "<p class=\"p\"><span>The earth brought forth.</span></p>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Non-Breaking Spaces
  {
    Webserver_Request request;
    string html = "<p class=\"p\"><span>The&nbsp;earth &nbsp; brought&nbsp;&nbsp;forth.</span></p>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Embedded Text Spans One
  {
    Webserver_Request request;
    string html = "<p class=\"p\"><span>The <span class=\"add\"><span class=\"nd\">Lord God</span> is calling</span> you</span><span>.</span></p>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The \\add \\+nd Lord God\\+nd* is calling\\add* you.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Basic note
  {
    Webserver_Request request;
    string html =
    "<p class=\"p\"><span>The earth brought forth</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">x</a><span>.</span></p>\n"
    "<div id=\"notes\">\n"
    "<hr>\n"
    "<p class=\"x\"><a href=\"#citation1\" id=\"note1\">x</a><span> </span><span>+ 2 Joh. 1.1</span></p>\n"
    "<br>\n"
    "</div>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth\\x + 2 Joh. 1.1\\x*.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Footnote Deleted Body
  {
    Webserver_Request request;
    string html =
    "<p class=\"p\"><span>The earth brought forth</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">f</a><span>.</span></p>\n"
    "<div id=\"notes\">\n"
    "<hr>\n"
    "<p class=\"f\"></p>\n"
    "<br>\n"
    "</div>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
    // Clear message from logbook.
    refresh_sandbox (false);
  }
  // Footnote Deleted Citation
  {
    Webserver_Request request;
    string html =
      "<p class=\"p\"><span>The earth brought forth</span><span>.</span></p>\n"
      "<div id=\"notes\">\n"
      "<hr>\n"
      "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">f</a><span> </span><span>+ </span><span class=\"fk\">brought: </span><span class=\"fl\">Heb. </span><span class=\"fq\">explanation.</span></p>\n"
      "</div>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
}


void test_editor_import () // Todo
{
  // Text Length One
  {
    string usfm =
      "\\c 2\n"
      "\\p\n"
      "\\v 1 Kwasekuqediswa amazulu lomhlaba lalo lonke ibutho lakho\\x + Dute. 4.19. Hlab. 33.6.\\x*.\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    evaluate (__LINE__, __func__, 61, editor_import.textLength);
    evaluate (__LINE__, __func__,  { make_pair (0, 0), make_pair (1, 2) }, editor_import.verseStartOffsets);
  }
  // Text Length More
  {
    string usfm =
    "\\c 2\n"
    "\\p\n"
    "\\v 1 Kwasekuqediswa amazulu lomhlaba lalo lonke ibutho lakho\\x + Dute. 4.19. Hlab. 33.6.\\x*.\n"
    "\\v 2 UNkulunkulu wasewuqeda ngosuku lwesikhombisa umsebenzi wakhe abewenza. Waphumula ngosuku lwesikhombisa\\x + Eks. 20.11; 31.17. Dute. 5.14. Heb. 4.4.\\x* emsebenzini wakhe wonke abewenza.\n"
    "\\v 3 UNkulunkulu wasebusisa usuku lwesikhombisa, walungcwelisa; ngoba ngalo waphumula emsebenzini wakhe wonke, uNkulunkulu awudalayo wawenza\\f + \\fk wawenza: \\fl Heb. \\fq ukuwenza.\\f*.\n"
    "\\s Isivande seEdeni\n"
    "\\p\n"
    "\\v 4 Lezi yizizukulwana zamazulu lezomhlaba ekudalweni kwakho\\x + 1.1.\\x*, mhla iN\\nd kosi\\nd* uNkulunkulu isenza umhlaba lamazulu,\n"
    "\\v 5 laso sonke isihlahlakazana sensimu, singakabi khona emhlabeni, layo yonke imibhida yeganga\\x + 1.12.\\x*, ingakamili; ngoba iN\\nd kosi\\nd* uNkulunkulu yayinganisanga izulu emhlabeni, njalo kwakungelamuntu wokulima umhlabathi.\n"
    "\\v 6 Kodwa kwenyuka inkungu ivela emhlabathini, yasithelela ubuso bonke bomhlabathi.\n"
    "\\v 7 IN\\nd kosi\\nd* uNkulunkulu yasibumba umuntu ngothuli oluvela emhlabathini\\x + 3.19,23. Hlab. 103.14. Tshu. 12.7. 1 Kor. 15.47.\\x*, yaphefumulela emakhaleni akhe umoya wempilo; umuntu wasesiba ngumphefumulo ophilayo\\x + 7.22. Jobe 33.4. Isa. 2.22. 1 Kor. 15.45.\\x*.\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    evaluate (__LINE__, __func__, 913, editor_import.textLength);
    evaluate (__LINE__, __func__, { make_pair (0, 0),
                                    make_pair (1, 2),
                                    make_pair (2, 62),
                                    make_pair (3, 202),
                                    make_pair (4, 359),
                                    make_pair (5, 469),
                                    make_pair (6, 676),
                                    make_pair (7, 758) },
                                    editor_import.verseStartOffsets);
  }
  // Space After Starting Marker
  {
    string usfm =
    "\\c 1\n"
    "\\p\n"
    "\\v 2 Text \\add of the \\add*1st\\add  second verse\\add*.\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    string standard =
    "<p class=\"c\"><span>1</span></p>"
    "<p class=\"p\"><span class=\"v\">2</span><span> </span><span>Text </span><span class=\"add\">of the </span><span>1st</span><span class=\"add\"> second verse</span><span>.</span></p>";
    evaluate (__LINE__, __func__, standard, html);
  }
}


void test_editor_roundtrip1 ()
{
  // One Unknown Marker Opener
  {
    string standard_usfm = "\\abc";
    string standard_html = "<p class=\"mono\"><span>\\abc </span></p>";
    
    Webserver_Request request;

    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, filter_string_trim (html));
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, filter_string_trim (usfm));
  }
  // Two Unknown Marker Openers
  {
    string standard_usfm =
    "\\abc\n"
    "\\abc";
    string standard_html =
    "<p class=\"mono\"><span>\\abc </span></p>"
    "<p class=\"mono\"><span>\\abc </span></p>";
    
    Webserver_Request request;

    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // One Unknown Markers Closer
  {
    string standard_usfm = "\\abc text\\abc*.";
    string standard_html = "<p class=\"mono\"><span>\\abc </span><span>text</span><span>\\abc*</span><span>.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Two Unknown Markers Closer
  {
    string standard_usfm =
    "\\abc text\\abc*.\n"
    "\\abc text\\abc*.";
    string standard_html =
    "<p class=\"mono\"><span>\\abc </span><span>text</span><span>\\abc*</span><span>.</span></p>"
    "<p class=\"mono\"><span>\\abc </span><span>text</span><span>\\abc*</span><span>.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Identifiers
  {
    string standard_usfm =
    "\\id GEN\n"
    "\\h Header\n"
    "\\toc1 The Book of Genesis\n"
    "\\cl Chapter\n"
    "\\cp ②\n"
    "\\cp Ⅰ";
    string standard_html =
    "<p class=\"mono\"><span>\\id </span><span>GEN</span></p>"
    "<p class=\"mono\"><span>\\h </span><span>Header</span></p>"
    "<p class=\"mono\"><span>\\toc1 </span><span>The Book of Genesis</span></p>"
    "<p class=\"mono\"><span>\\cl </span><span>Chapter</span></p>"
    "<p class=\"mono\"><span>\\cp </span><span>②</span></p>"
    "<p class=\"mono\"><span>\\cp </span><span>Ⅰ</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // One Paragraph
  {
    string standard_usfm = "\\p Paragraph text.";
    string standard_html = "<p class=\"p\"><span>Paragraph text.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Two Paragraphs
  {
    string standard_usfm =
    "\\p Paragraph text.\n"
    "\\p Paragraph txt.";
    string standard_html =
    "<p class=\"p\"><span>Paragraph text.</span></p>"
    "<p class=\"p\"><span>Paragraph txt.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet ("Standard");
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet ("Standard");
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  
}

/* Todo
Round trip tests.
 
 
 public function testInlineText ()
 {
 $standard_usfm = <<<'EOD'
 \p Paragraph text plus \add added\add* text.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>Paragraph text plus </span><span class="add">added</span><span> text.</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testInlineTexts ()
 {
 $standard_usfm = <<<'EOD'
 \p Paragraph text plus \add added\add* text plus \add added\add* text.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>Paragraph text plus </span><span class="add">added</span><span> text plus </span><span class="add">added</span><span> text.</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testChapter ()
 {
 $standard_usfm = <<<'EOD'
 \c 1
 \p Paragraph.
 EOD;
 $standard_html = <<<'EOD'
 <p class="c"><span>1</span></p>
 <p class="p"><span>Paragraph.</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testVerses ()
 {
 $standard_usfm = <<<'EOD'
 \p
 \v 1 One.
 \v 2 Two.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span class="v">1</span><span> </span><span>One.</span><span> </span><span class="v">2</span><span> </span><span>Two.</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testPeripherals ()
 {
 $standard_usfm = <<<'EOD'
 \periph Title Page
 \periph Publication Data
 EOD;
 $standard_html = <<<'EOD'
 <p class="mono"><span>\periph </span><span>Title Page</span></p>
 <p class="mono"><span>\periph </span><span>Publication Data</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testPicture ()
 {
 $standard_usfm = <<<'EOD'
 \p Text
 \fig DESC|FILE|SIZE|LOC|COPY|CAP|REF\fig*
 \p Text
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>Text</span></p>
 <p class="mono"><span>\fig </span><span>DESC|FILE|SIZE|LOC|COPY|CAP|REF</span><span>\fig*</span></p>
 <p class="p"><span>Text</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testTable ()
 {
 $standard_usfm = <<<'EOD'
 \tr \th1 Tribe \th2 Leader \thr3 Number
 \tr \tc1 Reuben \tc2 Elizur son of Shedeur \tcr3 46500
 \tr \tc1 Simeon \tc2 Shelumiel son of Zurishaddai \tcr3 59300
 \tr \tc1 Gad \tc2 Eliasaph son of Reuel \tcr3 45650
 \tr \tc1 \tcr2 Total: \tcr3 151450
 EOD;
 $standard_html = <<<'EOD'
 <p class="mono"><span>\tr </span><span class="th1">Tribe </span><span class="th2">Leader </span><span class="thr3">Number</span></p>
 <p class="mono"><span>\tr </span><span class="tc1">Reuben </span><span class="tc2">Elizur son of Shedeur </span><span class="tcr3">46500</span></p>
 <p class="mono"><span>\tr </span><span class="tc1">Simeon </span><span class="tc2">Shelumiel son of Zurishaddai </span><span class="tcr3">59300</span></p>
 <p class="mono"><span>\tr </span><span class="tc1">Gad </span><span class="tc2">Eliasaph son of Reuel </span><span class="tcr3">45650</span></p>
 <p class="mono"><span>\tr </span><span class="tcr2">Total: </span><span class="tcr3">151450</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $standard_usfm = <<<'EOD'
 \tr \th1 Tribe \th2 Leader \thr3 Number
 \tr \tc1 Reuben \tc2 Elizur son of Shedeur \tcr3 46500
 \tr \tc1 Simeon \tc2 Shelumiel son of Zurishaddai \tcr3 59300
 \tr \tc1 Gad \tc2 Eliasaph son of Reuel \tcr3 45650
 \tr \tcr2 Total: \tcr3 151450
 EOD;
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testWordListEntry ()
 {
 $standard_usfm = <<<'EOD'
 \p A \ndx index\ndx* b \wh Hebrew\wh* c.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>A </span><span class="ndx">index</span><span> b </span><span class="wh">Hebrew</span><span> c.</span></p>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testCrossreference ()
 {
 $standard_usfm = <<<'EOD'
 \p The elder\x + 2 Joh. 1.1\x* to the beloved Gaius.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>The elder</span><a href="#note1" id="citation1" class="superscript">1</a><span> to the beloved Gaius.</span></p>
 <div id="notes">
 <hr>
 <p class="x"><a href="#citation1" id="note1">1</a><span> </span><span>+ 2 Joh. 1.1</span></p>
 </div>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testCrossreferences ()
 {
 $standard_usfm = <<<'EOD'
 \p The elder\x + 2 Joh. 1.1\x* to the beloved Gaius.
 \v 1 The elders\x + 2 Joh. 2.2\x* to the beloved.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>The elder</span><a href="#note1" id="citation1" class="superscript">1</a><span> to the beloved Gaius.</span><span> </span><span class="v">1</span><span> </span><span>The elders</span><a href="#note2" id="citation2" class="superscript">2</a><span> to the beloved.</span></p>
 <div id="notes">
 <hr>
 <p class="x"><a href="#citation1" id="note1">1</a><span> </span><span>+ 2 Joh. 1.1</span></p>
 <p class="x"><a href="#citation2" id="note2">2</a><span> </span><span>+ 2 Joh. 2.2</span></p>
 </div>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testFootnote ()
 {
 $standard_usfm = <<<'EOD'
 \p The earth brought forth\f + \fk brought: \fl Heb. \fq explanation.\f*.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>The earth brought forth</span><a href="#note1" id="citation1" class="superscript">1</a><span>.</span></p>
 <div id="notes">
 <hr>
 <p class="f"><a href="#citation1" id="note1">1</a><span> </span><span>+ </span><span class="fk">brought: </span><span class="fl">Heb. </span><span class="fq">explanation.</span></p>
 </div>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testCyclingNoteCaller ()
 {
 $standard_usfm = <<<'EOD'
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 \p Text\f + note\f*.
 \p Text\fe + note\fe*.
 \p Text\x + note\x*.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>Text</span><a href="#note1" id="citation1" class="superscript">1</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note2" id="citation2" class="superscript">2</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note3" id="citation3" class="superscript">3</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note4" id="citation4" class="superscript">4</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note5" id="citation5" class="superscript">5</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note6" id="citation6" class="superscript">6</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note7" id="citation7" class="superscript">7</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note8" id="citation8" class="superscript">8</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note9" id="citation9" class="superscript">9</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note10" id="citation10" class="superscript">1</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note11" id="citation11" class="superscript">2</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note12" id="citation12" class="superscript">3</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note13" id="citation13" class="superscript">4</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note14" id="citation14" class="superscript">5</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note15" id="citation15" class="superscript">6</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note16" id="citation16" class="superscript">7</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note17" id="citation17" class="superscript">8</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note18" id="citation18" class="superscript">9</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note19" id="citation19" class="superscript">1</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note20" id="citation20" class="superscript">2</a><span>.</span></p>
 <p class="p"><span>Text</span><a href="#note21" id="citation21" class="superscript">3</a><span>.</span></p>
 <div id="notes">
 <hr>
 <p class="f"><a href="#citation1" id="note1">1</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation2" id="note2">2</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation3" id="note3">3</a><span> </span><span>+ note</span></p>
 <p class="f"><a href="#citation4" id="note4">4</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation5" id="note5">5</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation6" id="note6">6</a><span> </span><span>+ note</span></p>
 <p class="f"><a href="#citation7" id="note7">7</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation8" id="note8">8</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation9" id="note9">9</a><span> </span><span>+ note</span></p>
 <p class="f"><a href="#citation10" id="note10">1</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation11" id="note11">2</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation12" id="note12">3</a><span> </span><span>+ note</span></p>
 <p class="f"><a href="#citation13" id="note13">4</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation14" id="note14">5</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation15" id="note15">6</a><span> </span><span>+ note</span></p>
 <p class="f"><a href="#citation16" id="note16">7</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation17" id="note17">8</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation18" id="note18">9</a><span> </span><span>+ note</span></p>
 <p class="f"><a href="#citation19" id="note19">1</a><span> </span><span>+ note</span></p>
 <p class="fe"><a href="#citation20" id="note20">2</a><span> </span><span>+ note</span></p>
 <p class="x"><a href="#citation21" id="note21">3</a><span> </span><span>+ note</span></p>
 </div>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testEndnote ()
 {
 $standard_usfm = <<<'EOD'
 \p The earth brought forth\fe + \fk brought: \fl Heb. \fq explanation.\fe*.
 EOD;
 $standard_html = <<<'EOD'
 <p class="p"><span>The earth brought forth</span><a href="#note1" id="citation1" class="superscript">1</a><span>.</span></p>
 <div id="notes">
 <hr>
 <p class="fe"><a href="#citation1" id="note1">1</a><span> </span><span>+ </span><span class="fk">brought: </span><span class="fl">Heb. </span><span class="fq">explanation.</span></p>
 </div>
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($standard_usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $this->assertEquals ($standard_html, filter_string_trim ($html));
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($standard_html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $usfm = $editor_export->get ();
 $this->assertEquals ($standard_usfm, filter_string_trim ($usfm));
 }
 
 
 public function testRealisticRoundTrip1 ()
 {
 $usfm = <<<'EOD'
 \c 1
 \s Ukufika kukaJesu Kristu kuyamenyezelwa nguJohane uMbhabhathizi
 \p
 \v 1 Ukuqala kwevangeli likaJesu Kristu, iNdodana kaNkulunkulu\x + Mat. 14.33.\x*;
 \v 2 njengokulotshiweyo kubaprofethi\x + Mat. 3.1-11. Luka 3.2-16.\x* \add ukuthi\add*: Khangela, mina ngiyathuma isithunywa sami phambi kobuso bakho, esizalungisa indlela yakho phambi kwakho\x + Mal. 3.1. Mat. 11.10. Luka 7.27.\x*.
 \v 3 Ilizwi lomemezayo enkangala, \add lithi\add*: Lungisani indlela yeNkosi; qondisani izindledlana zayo\x + Isa. 40.3. Joha. 1.23.\x*.
 \v 4 Kweza uJohane ebhabhathiza enkangala, etshumayela ubhabhathizo lokuphenduka kukho ukuthethelelwa kwezono\x + Mat. 26.28. Luka 1.77.\x*,
 \v 5 njalo kwaphumela kuye ilizwe lonke leJudiya, labo abeJerusalema, basebebhabhathizwa bonke nguye emfuleni iJordani, bevuma izono zabo.
 \v 6 Futhi uJohane wayembethe uboya bekamela, elozwezwe lwesikhumba ekhalweni lwakhe, wayesidla intethe loluju lweganga.
 \p
 \v 7 Watshumayela, esithi: Olamandla kulami uyeza emva kwami, engingafanele ukukhothama ngithukulule umchilo wamanyathela akhe\x + Gen. 14.23. Isa. 5.27. Luka 3.16. Joha. 1.27.\x*.
 \v 7 Watshumayela, esithi: Olamandla kulami uyeza emva kwami, engingafanele ukukhothama ngithukulule umchilo wamanyathela akhe\x + Gen. 14.23. Isa. 5.27. Luka 3.16. Joha. 1.27.\x*.
 \v 8 Mina isibili ngilibhabhathize ngamanzi; kodwa yena uzalibhabhathiza ngoMoya oNgcwele.
 \s Ukubhabhathizwa kukaJesu
 \p
 \v 9 Kwasekusithi ngalezonsuku\x + Mat. 3.13-17. Luka 3.21,22.\x* kwafika uJesu evela eNazaretha yeGalili\x + Mat. 2.23.\x*, wabhabhathizwa nguJohane eJordani.
 \v 10 Wenela ukukhuphuka emanzini, wabona amazulu eqhekezeka, loMoya kungathi lijuba esehlela phezu kwakhe;
 \v 11 kwasekusiza ilizwi livela emazulwini, \add lathi\add*: Wena uyiNdodana yami ethandekayo, engithokoza ngayo.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $result = $editor_export->get ();
 $this->assertEquals ($usfm, filter_string_trim ($result));
 }
 
 
 public function testRealisticRoundTrip2 ()
 {
 $usfm = <<<'EOD'
 \c 1
 \s Ukudalwa komhlaba laye umuntu
 \p
 \v 1 Ekuqaleni\x + Hlab. 33.6; 136.5; Joha. 1.1-3; Seb. 14.15; 17.24; Kol. 1.16,17; Heb. 1.8-10; 11.3.\x* uNkulunkulu wadala amazulu lomhlaba.
 \v 2 Njalo umhlaba wawungelasimo ungelalutho\x + Isa. 34.11 Jer. 4.23.\x*, lobumnyama babuphezu kokujula; loMoya kaNkulunkulu wayendiza phezu kwamanzi.
 \v 3 UNkulunkulu wasesithi: Kakube khona ukukhanya\x + 2 Kor. 4.6\x*; kwasekusiba khona ukukhanya.
 \v 4 UNkulunkulu wasebona ukukhanya ukuthi kuhle; uNkulunkulu wasesehlukanisa phakathi kokukhanya lobumnyama.
 \v 5 UNkulunkulu wasebiza ukukhanya ngokuthi yimini, lobumnyama wabubiza ngokuthi yibusuku. Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lokuqala.
 \p
 \v 6 UNkulunkulu wasesithi: Kakube khona umkhathi\x + Jobe 37.18. Hlab. 136.5. Jer. 10.12; 51.15.\x* phakathi kwamanzi; kawehlukanise phakathi kwamanzi lamanzi.
 \v 7 UNkulunkulu wasesenza umkhathi, wasesehlukanisa phakathi kwamanzi angaphansi komkhathi lamanzi aphezu komkhathi\x + Hlab. 148.4.\x*; kwasekusiba njalo.
 \v 8 UNkulunkulu wasebiza umkhathi ngokuthi ngamazulu. Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesibili.
 \p
 \v 9 UNkulunkulu wasesithi: Kakuqoqelwe amanzi avela ngaphansi kwamazulu ndawonye\x + Jobe 38.8. Hlab. 33.7. 2 Pet. 3.5.\x*, njalo kakubonakale umhlabathi owomileyo; kwasekusiba njalo.
 \v 10 UNkulunkulu wasebiza umhlabathi owomileyo ngokuthi ngumhlaba, lenhlangano yamanzi wayibiza ngokuthi zinlwandle. UNkulunkulu wasebona ukuthi kuhle.
 \v 11 UNkulunkulu wasesithi: Umhlaba kawumilise utshani\f + \fk utshani: \fl Heb. \fq utshani obubuthakathaka.\f*, imibhida ethela inhlanyelo, isihlahla sesithelo sisenza isithelo ngohlobo lwaso, onhlanyelo yaso ikuso phezu komhlaba. Kwasekusiba njalo.
 \v 12 Umhlaba wasuveza utshani, imibhida ethela inhlanyelo ngohlobo lwayo, lesihlahla sisenza isithelo, esinhlanyelo yaso ikuso ngohlobo lwaso. UNkulunkulu wasebona ukuthi kuhle.
 \v 13 Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesithathu.
 \p
 \v 14 UNkulunkulu wasesithi: Kakube khona izikhanyiso emkhathini wamazulu, ukuze zehlukanise phakathi kwemini lobusuku\f + \fk phakathi kwemini lobusuku: \fl Heb. \fq phakathi kwemini laphakathi kobusuku.\f*; njalo zibe yizibonakaliso lezikhathi ezimisiweyo, lensuku leminyaka.
 \v 15 Njalo zibe yizikhanyiso emkhathini wamazulu, ukuze zikhanyise emhlabeni. Kwasekusiba njalo.
 \v 16 UNkulunkulu wasesenza izikhanyiso ezimbili ezinkulu\x + Dute. 4.19. Hlab. 136.7,8,9.\x*: Isikhanyiso esikhulu ukuze sibuse imini, lesikhanyiso esincinyane ukuze sibuse\f + \fk sibuse: \fl Heb. \fq kube yikubusa kwe-.\f* ubusuku, kanye lezinkanyezi.
 \v 17 UNkulunkulu wasezimisa emkhathini wamazulu, ukuthi zikhanyise emhlabeni,
 \v 18 lokuthi zibuse emini\x + Jer. 31.35\x* lebusuku, lokwehlukanisa phakathi kokukhanya lobumnyama. UNkulunkulu wasebona ukuthi kuhle.
 \v 19 Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesine.
 \p
 \v 20 UNkulunkulu wasesithi: Amanzi kawagcwale ukunyakazela kwemiphefumulo ephilayo, njalo kakuphaphe inyoni phezu komhlaba emkhathini\f + \fk emkhathini: \fl Heb. \fq ebusweni bomkhathi.\f* wamazulu.
 \v 21 UNkulunkulu wasedala oququmadevu bolwandle abakhulu, laso sonke isidalwa esiphilayo esibhinqikayo, amanzi agcwele sona, ngohlobo lwaso, layo yonke inyoni elempiko, ngohlobo lwayo. UNkulunkulu wasebona ukuthi kuhle.
 \v 22 UNkulunkulu wasekubusisa, esithi: Zalani, lande\x + 8.17; 9.1.\x*, njalo ligcwalise amanzi enlwandle, lezinyoni kazande emhlabeni.
 \v 23 Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesihlanu.
 \p
 \v 24 UNkulunkulu wasesithi: Umhlaba kawuveze izidalwa eziphilayo ngohlobo lwazo, izifuyo lokuhuquzelayo lenyamazana zomhlaba ngenhlobo zazo. Kwasekusiba njalo.
 \v 25 UNkulunkulu wasesenza izinyamazana zomhlaba ngenhlobo zazo, lezifuyo ngenhlobo zazo, lakho konke okuhuquzelayo komhlaba ngenhlobo zakho. UNkulunkulu wasebona ukuthi kuhle.
 \p
 \v 26 UNkulunkulu wasesithi: Asenze abantu ngomfanekiso wethu\x + 5.1; 9.6. 1 Kor. 11.7. Efe. 4.24. Kol. 3.10. Jak. 3.9.\x*, ngesimo sethu, njalo babuse phezu kwenhlanzi zolwandle, laphezu kwenyoni zamazulu, laphezu kwezifuyo, laphezu komhlaba wonke, laphezu kwakho konke okuhuquzelayo lokho okuhuquzela emhlabeni\x + 9.2. Hlab. 8.6.\x*.
 \v 27 UNkulunkulu wasemdala umuntu ngomfanekiso wakhe; wamdala ngomfanekiso kaNkulunkulu; owesilisa lowesifazana wabadala\x + 2.18; 5.2. Mal. 2.15. Mat. 19.4. Mark. 10.6.\x*.
 \v 28 UNkulunkulu wasebabusisa, uNkulunkulu wasesithi kibo: Zalani lande, ligcwalise umhlaba\x + 9.1,7.\x* njalo liwehlisele phansi; libuse phezu kwenhlanzi zolwandle, laphezu kwenyoni zamazulu, laphezu kwakho konke okuphilayo okuhuquzela emhlabeni.
 \p
 \v 29 UNkulunkulu wasesithi: Khangelani, ngilinike yonke imibhida ethela inhlanyelo, esebusweni bomhlaba wonke, laso sonke isihlahla, okukuso isithelo sesihlahla esithela\f + \fk esithela: \fl Heb. \fq esihlanyela.\f* inhlanyelo; kini singesokudla\x + 9.3. Hlab. 145.15,16.\x*.
 \v 30 Njalo kuyo yonke inyamazana yomhlaba, lakuyo yonke inyoni yamazulu, lakukho konke okuhuquzela emhlabeni, okukukho umoya\f + \fk umoya: \fl Heb. \fq umphefumulo.\f* ophilayo, lonke uhlaza ngolokudla. Kwasekusiba njalo.
 \v 31 UNkulunkulu wasebona konke akwenzileyo; khangela-ke kwakukuhle kakhulu\x + Tshu. 7.29. 1 Tim. 4.4.\x*. Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesithupha.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $result = $editor_export->get ();
 $this->assertEquals ($usfm, filter_string_trim ($result));
 }
 
 
 public function testMissingNoteCaller ()
 {
 $usfm = <<<'EOD'
 \p The earth brought forth\f + \fk brought: \fl Heb. \fq explanation.\f*.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $html = str_replace ("+", " ", $html);
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $result = $editor_export->get ();
 $this->assertEquals ($usfm, filter_string_trim ($result));
 }
 
 
 public function testMissingNoteCallers ()
 {
 $usfm = <<<'EOD'
 \p The earth brought forth\f + \fk brought: \fl Heb. \fq explanation.\f*.
 \p The earth brought forth\f + \fk brought: \fl Heb. \fq explanation.\f*.
 \p The earth brought forth\f + \fk brought: \fl Heb. \fq explanation.\f*.
 \p The earth brought forth\x + \fk brought: \fl Heb. \fq explanation.\x*.
 \p The earth brought forth\fe + \fk brought: \fl Heb. \fq explanation.\fe*.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $html = $editor_import->get ();
 $html = str_replace ("+", " ", $html);
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $result = $editor_export->get ();
 $this->assertEquals ($usfm, filter_string_trim ($result));
 }
 
 
 public function testNestedTextMarkup1 ()
 {
 $usfm = <<<'EOD'
 \p
 \v 2 \add add\+nd addnd\+nd*\add*.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $output = $editor_import->get ();
 $html = <<<'EOD'
 <p class="p"><span class="v">2</span><span> </span><span class="add">add</span><span class="add nd">addnd</span><span>.</span></p>
 EOD;
 $this->assertEquals ($html, $output);
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $output = $editor_export->get ();
 $usfm = <<<'EOD'
 \p
 \v 2 \add add\add*\add \+nd addnd\+nd*\add*.
 EOD;
 $this->assertEquals ($usfm, $output);
 }
 
 
 public function testNestedTextMarkup2 ()
 {
 $usfm = <<<'EOD'
 \p
 \v 2 \add add\+nd addnd\add*.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $output = $editor_import->get ();
 $html = <<<'EOD'
 <p class="p"><span class="v">2</span><span> </span><span class="add">add</span><span class="add nd">addnd</span><span>.</span></p>
 EOD;
 $this->assertEquals ($html, $output);
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $output = $editor_export->get ();
 $usfm = <<<'EOD'
 \p
 \v 2 \add add\add*\add \+nd addnd\+nd*\add*.
 EOD;
 $this->assertEquals ($usfm, $output);
 }
 
 
 public function testNestedTextMarkup3 ()
 {
 $usfm = <<<'EOD'
 \p The \add \+nd Lord God\+nd* is\add* calling you
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $output = $editor_import->get ();
 $html = <<<'EOD'
 <p class="p"><span>The </span><span class="add nd">Lord God</span><span class="add"> is</span><span> calling you</span></p>
 EOD;
 $this->assertEquals ($html, $output);
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $output = $editor_export->get ();
 $usfm = <<<'EOD'
 \p The \add \+nd Lord God\+nd*\add*\add  is\add* calling you
 EOD;
 $this->assertEquals ($usfm, $output);
 }
 
 
 public function testNestedNoteMarkup1 ()
 {
 $usfm = <<<'EOD'
 \p
 \v 2 text\f + \fk fk \+fdc key-fdc\+fdc*\fk* normal\f*.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $output = $editor_import->get ();
 $html = <<<'EOD'
 <p class="p"><span class="v">2</span><span> </span><span>text</span><a href="#note1" id="citation1" class="superscript">1</a><span>.</span></p>
 <div id="notes">
 <hr>
 <p class="f"><a href="#citation1" id="note1">1</a><span> </span><span>+ </span><span class="fk">fk </span><span class="fk fdc">key-fdc</span><span> normal</span></p>
 </div>
 EOD;
 $this->assertEquals ($html, $output);
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $output = $editor_export->get ();
 $usfm = <<<'EOD'
 \p
 \v 2 text\f + \fk fk \fk \+fdc key-fdc\+fdc*\fk* normal\f*.
 EOD;
 $this->assertEquals ($usfm, $output);
 }
 
 
 public function testNestedNoteMarkup2 ()
 {
 $usfm = <<<'EOD'
 \p
 \v 2 text\f + \fk fk \+fdc key-fdc\fk* normal\f*.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $output = $editor_import->get ();
 $html = <<<'EOD'
 <p class="p"><span class="v">2</span><span> </span><span>text</span><a href="#note1" id="citation1" class="superscript">1</a><span>.</span></p>
 <div id="notes">
 <hr>
 <p class="f"><a href="#citation1" id="note1">1</a><span> </span><span>+ </span><span class="fk">fk </span><span class="fk fdc">key-fdc</span><span> normal</span></p>
 </div>
 EOD;
 $this->assertEquals ($html, $output);
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $output = $editor_export->get ();
 $usfm = <<<'EOD'
 \p
 \v 2 text\f + \fk fk \fk \+fdc key-fdc\+fdc*\fk* normal\f*.
 EOD;
 $this->assertEquals ($usfm, $output);
 }
 
 
 public function testFixForChangeFt2Fk ()
 {
 // A bug was discovered in the Bible editor where "... \fk ... \ft ..." was changed to "... \fk ... \fk ...".
 // The bug was fixed.
 // This test would catch any regression.
 $usfm = <<<'EOD'
 \c 1
 \v 1 Canonical text\f + \fr 1:1 \fk Footnote fk style \ft Footnote ft style\f* canonical text.
 EOD;
 $editor_import = Editor_Import::getInstance ();
 $editor_import->load ($usfm);
 $editor_import->stylesheet ("Standard");
 $editor_import->run ();
 $output = $editor_import->get ();
 $html = <<<'EOD'
 <p class="c"><span>1</span><span> </span><span class="v">1</span><span> </span><span>Canonical text</span><a href="#note1" id="citation1" class="superscript">1</a><span> canonical text.</span></p>
 <div id="notes">
 <hr>
 <p class="f"><a href="#citation1" id="note1">1</a><span> </span><span>+ </span><span class="fr">1:1 </span><span class="fk">Footnote fk style </span><span class="ft">Footnote ft style</span></p>
 </div>
 EOD;
 $this->assertEquals ($html, $output);
 
 $editor_export = Editor_Export::getInstance ();
 $editor_export->load ($html);
 $editor_export->stylesheet ("Standard");
 $editor_export->run ();
 $output = $editor_export->get ();
 $this->assertEquals ($usfm, $output);
 }
 
 

 

 */
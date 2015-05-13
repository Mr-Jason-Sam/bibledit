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


#include <search/search.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/header.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/passage.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <database/notes.h>
#include <database/config/general.h>
#include <access/bible.h>


string search_search_url ()
{
  return "search/search";
}


bool search_search_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string search_search (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  
  string page;
 
  
  Assets_Header header = Assets_Header (translate("Search"), request);

  
  // The query: The word or string to search for.
  string queryString;
  if (request->query.count ("q")) {
    queryString = request->query ["q"];
  }
  // Put the query string into the search box.
  header.setSearchQuery (queryString);

  
  page = header.run ();

  
  Assets_View view = Assets_View ();
  
  
  // Clean the query string up.
  queryString = filter_string_trim (queryString);
  
  
  // Generate search words for emphasizing the search hits.
  vector <string> queryWords = filter_string_explode (queryString, ' ');
  
  
  Database_Notes database_notes = Database_Notes (request);

  
  string siteUrl = Database_Config_General::getSiteURL ();

  
  vector <string> bibles = access_bible_bibles (request);


  // Search the notes.
  vector <int> identifiers = database_notes.searchNotes (queryString, bibles);
  
  
  int noteCount = identifiers.size();
  view.set_variable ("noteCount", convert_to_string (noteCount));
  
  
  // Assemble the block of search results.
  string notesblock;
  for (auto identifier : identifiers) {
    
    // The title.
    string summary = database_notes.getSummary (identifier);
    string verses = filter_passage_display_inline (database_notes.getPassages (identifier));
    string title = summary + " | " + verses;
    title = filter_string_sanitize_html (title);
    
    // The url.
    string url = siteUrl + "/notes/note?id=" + convert_to_string (identifier);
    
    // The excerpt.
    string stext = database_notes.getSearchField (identifier);
    vector <string> vtext = filter_string_explode (stext, '\n');
    string excerpt;
    // Go through each line of text separately.
    for (auto & line : vtext) {
      string markedLine = filter_string_markup_words (queryWords, line);
      // If the line is marked up, add it to the excerpts.
      if (!excerpt.empty()) excerpt.append ("\n");
      if (markedLine != line) {
        excerpt.append ("<p style=\"margin-top: 0em\">" + markedLine + "</p>");
      }
    }
    
    // The html to display.
    string html = "<p style=\"margin-top: 0.75em; margin-bottom: 0em\"><a href=\"";
    html.append (url);
    html.append ("\">");
    html.append (title);
    html.append ("</a></p>");
    html.append (excerpt);
    if (!notesblock.empty ()) notesblock.append ("\n");
    notesblock.append (html);
  }
 
  
  // Display the search results for the notes.
  view.set_variable ("notesblock", notesblock);
  
  
  // Search the Bible text.
  vector <int> hits = request->database_search()->searchText (queryString, bibles);
  
  
  int textCount = hits.size ();
  view.set_variable ("textCount", convert_to_string (textCount));
  
  
  // Assemble the search results.
  string textblock;
  for (auto & hit : hits) {
    // Get the details of this search hit.
    Passage details = request->database_search()->getBiblePassage (hit);
    string bible = details.bible;
    int book = details.book;
    int chapter = details.chapter;
    string verse = details.verse;
    // The title plus link.
    string link = bible + " | " + filter_passage_link_for_opening_editor_at (book, chapter, verse);
    // The excerpt.
    string stext = request->database_search()->getBibleVerseText (bible, book, chapter, convert_to_int (verse));
    vector <string> vtext = filter_string_explode (stext, '\n');
    string excerpt;
    // Go through each line of text separately.
    for (auto & line : vtext) {
      string markedLine = filter_string_markup_words (queryWords, line);
      if (markedLine != line) {
        // Store this bit of the excerpt.
        excerpt.append ("<p style=\"margin-top: 0em\">" + markedLine + "</p>\n");
      }
    }
    
    if (!textblock.empty ()) textblock.append ("\n");
    textblock.append ("<p style=\"margin-top: 0.75em; margin-bottom: 0em\">");
    textblock.append (link);
    textblock.append ("</p>");
    textblock.append (excerpt);
  }
  
  
  // Display the search results for the Bible text.
  view.set_variable ("textblock", textblock);
  
  
  page += view.render ("search", "search");
  
  
  page += Assets_Page::footer ();
  
  
  return page;
}


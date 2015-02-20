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


#include <navigation/passage.h>
#include <filter/string.h>
#include <filter/usfm.h>
#include <database/versifications.h>
#include <database/navigation.h>
#include <database/books.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <ipc/focus.h>


/*
 On the iPad, Bibledit uses the UIWebView class as the web browser.
 https://developer.apple.com/library/ios/documentation/UIKit/Reference/UIWebView_Class/
 Bibledit used to have a verse navigator that used overlays for selecting book, chapter, and verse.
 When selecting the verse, the UIWebView crashed with the following message:
 UIPopoverPresentationController should have a non-nil sourceView or barButtonItem set before the presentation occurs.
 This is a bug in the UIWebView.
 To circumvent that bug, and also for better operation on the iPad with regard to touch operation,
 Bibledit now uses a different verse navigator, one without the overlays.
*/


string Navigation_Passage::getNavigator (void * webserver_request, string bible)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  Database_Navigation database_navigation = Database_Navigation ();
  
  string user = request->session_logic()->currentUser ();
  
  string fragment;
  
  // Links to go back and forward are grayed out or active depending on available passages to go to.
  fragment.append ("<li>");
  if (database_navigation.previousExists (user)) {
    fragment.append ("<a id=\"navigateback\" href=\"navigateback\" title=\"" + translate("Back") + "\">↶</a>");
  } else {
    fragment.append ("<span class=\"grayedout\">↶</span>");
  }
  fragment.append ("</li>");
  fragment.append ("<li>");
  fragment.append (" ");
  if (database_navigation.nextExists (user)) {
    fragment.append ("<a id=\"navigateforward\" href=\"navigateforward\" title=\"" + translate("Forward") + "\">↷</a>");
  } else {
    fragment.append ("<span class=\"grayedout\">↷</span>");
  }
  fragment.append ("</li>");
  fragment.append ("\n");
  
  int book = Ipc_Focus::getBook (request);
  
  // The book should exist in the Bible.
  if (bible != "") {
    vector <int> books = request->database_bibles()->getBooks (bible);
    if (find (books.begin(), books.end(), book) == books.end()) {
      if (!books.empty ()) book = books [0];
      else book = 0;
    }
  }
  
  string bookName = Database_Books::getEnglishFromId (book);

  fragment.append ("<li><a id=\"selectbook\" href=\"selectbook\" title=\"" + translate ("Select book") + "\">" + bookName + "</a></li>");
  
  int chapter = Ipc_Focus::getChapter (request);
  
  // The chapter should exist in the book.
  if (bible != "") {
    vector <int> chapters = request->database_bibles()->getChapters (bible, book);
    if (find (chapters.begin(), chapters.end(), chapter) == chapters.end()) {
      if (!chapters.empty()) chapter = chapters [0];
      else chapter = 1;
    }
  }

  fragment.append ("<li><a id=\"selectchapter\" href=\"selectchapter\" title=\"" + translate ("Select chapter") + "\"> " + to_string (chapter) +  " </a></li>");
  
  fragment.append ("<li><span>:</span></li>");

  int verse = Ipc_Focus::getVerse (request);
  
  fragment.append ("<li><a id=\"selectverse\" href=\"selectverse\" title=\"" + translate ("Select verse") + "\"> " + to_string (verse) +  " </a></li>");

  // The result.
  return fragment;
}


string Navigation_Passage::getBooksFragment (void * webserver_request, string bible)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  int activeBook = Ipc_Focus::getBook (request);
  // Take standard books in case of no Bible.
  vector <int> books;
  if (bible == "") {
    books = Database_Books::getIDs ();
  } else {
    books = filter_passage_get_ordered_books (bible);
  }
  string html;
  for (auto book : books) {
    string bookName = Database_Books::getEnglishFromId (book);
    bool selected = (book == activeBook);
    getSelectorLink (html, to_string (book), "applybook", translate ("Select book"), bookName, selected);
  }
  getSelectorLink (html, "cancel", "applybook", translate ("Cancel"), "[" + translate ("cancel") + "]", false);

  html.insert (0, "<span id=\"applybook\">" + translate ("Select book"));
  html.append ("</span>");

  html.insert (0, "<li>");
  html.append ("</li>");
  
  return html;
}


string Navigation_Passage::getChaptersFragment (void * webserver_request, string bible, int book, int chapter)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  vector <int> chapters;
  if (bible.empty ()) {
    Database_Versifications database_versifications = Database_Versifications ();
    chapters = database_versifications.getChapters ("English", book, true);
  } else {
    chapters = request->database_bibles()->getChapters (bible, book);
  }
  string html;
  for (auto ch : chapters) {
    bool selected = (ch == chapter);
    getSelectorLink (html, to_string (ch), "applychapter", translate ("Select chapter"), to_string (ch), selected);
  }
  getSelectorLink (html, "cancel", "applychapter", translate ("Cancel"), "[" + translate ("cancel") + "]", false);

  html.insert (0, "<span id=\"applychapter\">" + translate ("Select chapter"));
  html.append ("</span>");

  html.insert (0, "<li>");
  html.append ("</li>");
  
  return html;
}


string Navigation_Passage::getVersesFragment (void * webserver_request, string bible, int book, int chapter, int verse)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  vector <int> verses;
  if (bible == "") {
    Database_Versifications database_versifications = Database_Versifications ();
    verses = database_versifications.getVerses ("English", book, chapter);
  } else {
    verses = usfm_get_verse_numbers (request->database_bibles()->getChapter (bible, book, chapter));
  }
  string html;
  for (auto vs : verses) {
    bool selected = (verse == vs);
    getSelectorLink (html, to_string (vs), "applyverse", translate ("Select verse"), to_string (vs), selected);
  }
  getSelectorLink (html, "cancel", "applyverse", translate ("Cancel"), "[" + translate ("cancel") + "]", false);

  html.insert (0, "<span id=\"applyverse\">" + translate ("Select verse"));
  html.append ("</span>");

  html.insert (0, "<li>");
  html.append ("</li>");
  
  return html;
}


string Navigation_Passage::code (string bible, bool header)
{
  string code = "";
  if (header) {
    code += "<script type=\"text/javascript\" src=\"/navigation/passage.js\"></script>";
    code += "\n";
  }
  code += "<script type=\"text/javascript\">";
  code += "\n";
  code += "navigationBible = \"" + bible + "\";";
  code += "\n";
  code += "</script>";
  code += "\n";
  return code;
}


void Navigation_Passage::setBook (void * webserver_request, int book)
{
  Ipc_Focus::set (webserver_request, book, 1, 1);
  recordHistory (webserver_request, book, 1, 1);
}


void Navigation_Passage::setChapter (void * webserver_request, int chapter)
{
  int book = Ipc_Focus::getBook (webserver_request);
  Ipc_Focus::set (webserver_request, book, chapter, 1);
  recordHistory (webserver_request, book, chapter, 1);
}


void Navigation_Passage::setVerse (void * webserver_request, int verse)
{
  int book = Ipc_Focus::getBook (webserver_request);
  int chapter = Ipc_Focus::getChapter (webserver_request);
  Ipc_Focus::set (webserver_request, book, chapter, verse);
  recordHistory (webserver_request, book, chapter, verse);
}


void Navigation_Passage::setBookChapterVerse (void * webserver_request, int book, int chapter, int verse)
{
  Ipc_Focus::set (webserver_request, book, chapter, verse);
  recordHistory (webserver_request, book, chapter, verse);
}


void Navigation_Passage::setPassage (void * webserver_request, string bible, string passage)
{
  int currentBook = Ipc_Focus::getBook (webserver_request);
  int currentChapter = Ipc_Focus::getChapter (webserver_request);
  int currentVerse = Ipc_Focus::getVerse (webserver_request);
  passage = filter_string_trim (passage);
  Passage passage_to_set;
  if ((passage == "") || (passage == "+")) {
    passage_to_set = Navigation_Passage::getNextVerse (webserver_request, bible, currentBook, currentChapter, currentVerse);
  } else if (passage == "-") {
    passage_to_set = Navigation_Passage::getPreviousVerse (webserver_request, bible, currentBook, currentChapter, currentVerse);
  } else {
    Passage inputpassage = Passage ("", currentBook, currentChapter, convert_to_string (currentVerse));
    passage_to_set = filter_passage_interpret_passage (inputpassage, passage);
  }
  if (passage_to_set.book != 0) {
    Ipc_Focus::set (webserver_request, passage_to_set.book, passage_to_set.chapter, convert_to_int (passage_to_set.verse));
    Navigation_Passage::recordHistory (webserver_request, passage_to_set.book, passage_to_set.chapter, convert_to_int (passage_to_set.verse));
  }
}


Passage Navigation_Passage::getNextVerse (void * webserver_request, string bible, int book, int chapter, int verse)
{
  verse++;
  if (bible != "") {
    Webserver_Request * request = (Webserver_Request *) webserver_request;
    vector <int> verses = usfm_get_verse_numbers (request->database_bibles()->getChapter (bible, book, chapter));
    if (find (verses.begin(), verses.end(), verse) == verses.end()) {
      if (!verses.empty()) verse = verses.back ();
    }
  }
  Passage passage = Passage ("", book, chapter, convert_to_string (verse));
  return passage;
}


Passage Navigation_Passage::getPreviousVerse (void * webserver_request, string bible, int book, int chapter, int verse)
{
  verse--;
  if (bible != "") {
    Webserver_Request * request = (Webserver_Request *) webserver_request;
    vector <int> verses = usfm_get_verse_numbers (request->database_bibles()->getChapter (bible, book, chapter));
    if (find (verses.begin(), verses.end(), verse) == verses.end()) {
      if (!verses.empty ()) verse = verses [0];
    }
  }
  Passage passage = Passage ("", book, chapter, convert_to_string (verse));
  return passage;
}


void Navigation_Passage::gotoNextVerse (void * webserver_request, string bible)
{
  int currentBook = Ipc_Focus::getBook (webserver_request);
  int currentChapter = Ipc_Focus::getChapter (webserver_request);
  int currentVerse = Ipc_Focus::getVerse (webserver_request);
  Passage passage = Navigation_Passage::getNextVerse (webserver_request, bible, currentBook, currentChapter, currentVerse);
  if (passage.book != 0) {
    Ipc_Focus::set (webserver_request, passage.book, passage.chapter, convert_to_int (passage.verse));
    Navigation_Passage::recordHistory (webserver_request, passage.book, passage.chapter, convert_to_int (passage.verse));
  }
}


void Navigation_Passage::gotoPreviousVerse (void * webserver_request, string bible)
{
  int currentBook = Ipc_Focus::getBook (webserver_request);
  int currentChapter = Ipc_Focus::getChapter (webserver_request);
  int currentVerse = Ipc_Focus::getVerse (webserver_request);
  Passage passage = Navigation_Passage::getPreviousVerse (webserver_request, bible, currentBook, currentChapter, currentVerse);
  if (passage.book != 0) {
    Ipc_Focus::set (webserver_request, passage.book, passage.chapter, convert_to_int (passage.verse));
    Navigation_Passage::recordHistory (webserver_request, passage.book, passage.chapter, convert_to_int (passage.verse));
  }
}


void Navigation_Passage::recordHistory (void * webserver_request, int book, int chapter, int verse)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  string user = request->session_logic()->currentUser ();
  Database_Navigation database_navigation = Database_Navigation ();
  database_navigation.record (filter_string_date_seconds_since_epoch (), user, book, chapter, verse);
}


void Navigation_Passage::goBack (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  Database_Navigation database_navigation = Database_Navigation ();
  string user = request->session_logic()->currentUser ();
  Passage passage = database_navigation.getPrevious (user);
  if (passage.book) {
    Ipc_Focus::set (webserver_request, passage.book, passage.chapter, convert_to_int (passage.verse));
  }
}


void Navigation_Passage::goForward (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  Database_Navigation database_navigation = Database_Navigation ();
  string user = request->session_logic()->currentUser ();
  Passage passage = database_navigation.getNext (user);
  if (passage.book) {
    Ipc_Focus::set (webserver_request, passage.book, passage.chapter, convert_to_int (passage.verse));
  }
}


void Navigation_Passage::getSelectorLink (string& html, string id, string href, string title, string text, bool selected)
{
  if (!html.empty ()) html.append ("|");
  if (selected) html.append ("<mark>");
  html.append ("<a id=\"" + id + "apply\" href=\"" + href + "\" title=\"" + title + "\"> ");
  html.append (text);
  html.append (" </a>");
  if (selected) html.append ("</mark>");
}


/*
public static function getEntry ()
{
  $html = "";
  $html .= '<input name="selectpassage" id="selectpassage" type="text" value=""/>';
  $html .= '<input name="submitpassage" id="submitpassage"  type="submit" value="' . gettext ("OK") . '" />';
  return $html;
}


public static function setPassage ($bible, $passage)
{
  $ipc_focus = Ipc_Focus::getInstance();
  $currentBook = $ipc_focus->getBook ();
  $currentChapter = $ipc_focus->getChapter ();
  $currentVerse = $ipc_focus->getVerse ();
  $passage = trim ($passage);
  if (($passage == "") || ($passage == "+")) {
    $passage = Navigation_Passage::getNextVerse ($bible, $currentBook, $currentChapter, $currentVerse);
  } else if ($passage == "-") {
    $passage = Navigation_Passage::getPreviousVerse ($bible, $currentBook, $currentChapter, $currentVerse);
  } else {
    $passage = Filter_Books::interpretPassage (array ($currentBook, $currentChapter, $currentVerse), $passage);
  }
  if ($passage[0] != 0) {
    $ipc_focus->set ($passage [0], $passage [1], $passage [2]);
    Navigation_Passage::recordHistory ($passage [0], $passage [1], $passage [2]);
    
  }
}
*/

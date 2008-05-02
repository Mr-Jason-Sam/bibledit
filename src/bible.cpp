/*
** Copyright (©) 2003-2008 Teus Benschop.
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/


#include "libraries.h"
#include "bible.h"
#include "utilities.h"
#include "unixwrappers.h"
#include "projectutils.h"
#include "books.h"
#include "settings.h"
#include "languages.h"
#include "settings.h"
#include "localizedbooks.h"
#include "tiny_utilities.h"


void quick_swap2 (unsigned int& a, unsigned int& b)
{
  unsigned int t = a;
  a = b;
  b = t; 
}


void quick_swap (Reference& a, Reference& b)
{
  Reference t = a;
  a = b;
  b = t; 
}


void quick_sort (vector<unsigned int>& one, vector<Reference>& two, unsigned int beg, unsigned int end)
{
  if (end > beg + 1) {
    unsigned int piv = one[beg];
    unsigned int l = beg + 1;
    unsigned int r = end;
    while (l < r) {
      if (one[l] <= piv) {
        l++;
      } else {
        --r;
        quick_swap2 (one[l], one[r]);
        quick_swap (two[l], two[r]);
      }
    }
    --l;
    quick_swap2 (one[l], one[beg]);
    quick_swap (two[l], two[beg]);
    quick_sort (one, two, beg, l);
    quick_sort (one, two, r, end);
  }
}


void sort_references (vector <Reference>& references)
/*
Sorts all references from Genesis to Revelation. 
Sorts on book first, then chapter, and finally verse.
*/
{
  // When there are less than two entries no sorting is needed.
  if (references.size () < 2) return;
  try 
  {
    // Make a vector that contains the numerical equivalent of the references.
    vector <unsigned int> numerical;
    for (unsigned int i = 0; i < references.size (); i++) {
      numerical.push_back (reference_to_numerical_equivalent (references[i]));
    }
    // Sort the references.
    quick_sort (numerical, references, 0, numerical.size());
  }
  catch (exception & ex)
  {
    cerr << "Sorting references: " << ex.what () << endl;
  }
}


void decode_reference (const ustring & reference, ustring & book, ustring & chapter, ustring & verse)
/*
 * Decodes "reference" and provides:
 * - book
 * - chapter
 * - verse
 * 
 * E.g. "Song of Solomon 1:1" becomes "Song of Solomon", 
 * chapter "1" and verse "1".
 */
{
  try
  {
    ustring ref (reference);
    // Extract the book.
    // Deal with books like "1 Samuel" or "Song of Solomon".
    int booklength;
    booklength = ref.rfind (" ");
    book = ref.substr (0, booklength);
    ref.erase (0, booklength);
    ref = trim (ref);
    // Extract chapter.
    chapter = number_in_string (ref);
    ref.erase (0, chapter.length () + 1);
    ref = trim (ref);
    // Extract verse.
    verse = ref;
  }
  catch (exception & ex)
  {
    cerr << ex.what () << endl;
  }
}


bool reference_discover_internal (unsigned int oldbook, unsigned int oldchapter, 
                                  const ustring& oldverse,  const ustring& reference,
                                  unsigned int& newbook, unsigned int& newchapter, ustring& newverse)
/*
This interprets "reference" as a valid reference.
If needed it will use information of the current reference to complete the info.
For example, when "reference" is "1", it is interpreted as verse one of the current
chapter of the current book. If "reference" is "21 10" it is interpreted as the current
book, chapter 21 verse 10. And so forth.
*/
{
  // Filter the response.
  string response = trim (reference);
  // Change a colon to a space.
  size_t position = response.find (":");
  while (position != string::npos) {
    response[position] = ' ';
    position = response.find (":");
  }
  // Change a dot to a space.
  // Dots are in OSIS references.
  position = response.find (".");
  while (position != string::npos) {
    response[position] = ' ';
    position = response.find (".");
  }
  // Change to upper case.
  response = upperCase (response);
  // Trim again.
  response = trim (response);
  // Divide the response in parts.
  // A special algorithm ensures that the book is properly formed.
  // Think of "1 Corinthians 10:1" and "Song of Songs".
  vector <ustring> input;
  {
    Parse parse (response);
    int highest_text_offset = -1;
    for (unsigned int i = 0; i < parse.words.size (); i++) {
      if (number_in_string (parse.words[i]) != parse.words[i]) {
        highest_text_offset = i;
      }
    }
    ustring bookpart;
    vector <ustring> numbers;
    for (unsigned int i = 0; i < parse.words.size (); i++) {
      if ((int)i > highest_text_offset) {
        numbers.push_back (parse.words[i]);
      } else {
        if (!bookpart.empty ()) bookpart.append (" ");
        bookpart.append (parse.words[i]);
      }
    }
    if (!bookpart.empty ()) {
      input.push_back (bookpart);
    }
    for (unsigned int i = 0; i < numbers.size (); i++) {
      input.push_back (numbers[i]);
    }
  }

  // See whether the first one is a name or a number.
  // If it's a name, then this will be the bookname.
  // If it's a number, there will be no bookname, but we have chapter and/or verse only.
  // If it contains a hyphen (-) or a comma (,) it is a verse number.
  bool book_was_given = false;
  if (input.size () > 0) {
    if (looks_like_verse (input[0])) {
      // It's a verse, so no bookname given. Use the current book.
      newbook = oldbook;
    }
    else {
      // It's a bookname, find out which.
      book_was_given = true;
      newbook = book_find_valid (input[0]);
      if (newbook == 0) {
        // If something like "1 co" has been added, deal with that by making it "1co", and checking again.
        ustring s = input[0];
        if (s.length () > 2) {
          if (s[1] == ' ') {
            s.erase (1, 1);
            newbook = book_find_valid (s);
          }
        }
        if (newbook == 0) {
          // Bad book: bail out.
          return false;
        }
      }
      // Remove the book, because it's no longer needed.
      input.erase (input.begin ());
    }
  }
  else {
    return false;
  }
  // As from here on we can be sure of this one thing: We've got a proper bookname.
  // Also as the bookname has been removed from the input data, only chapter and/or verse remain to be dealt with.
  if (input.size () >= 2)
  {
    // Two variables left, so that will be chapter and verse.
    ustring ch = number_in_string (input[0]);
    newverse = lowerCase (input[1]);
    if (ch.empty () || newverse.empty ())
    {
      return false;
    }
    newchapter = convert_to_int (ch);
    return true;
  }
  // Here we have only one variable left.
  // This is either chapter (as in "Genesis 1") or verse (as in "1").
  // Or we have no variable left, then we take chapter one verse one.
  if (book_was_given)
  {
    // Here deal with the fact that a book was given, but nothing else.
    if (input.size () == 0)
    {
      newchapter = 1;
      newverse = "1";
      return true;
    }
  }
  // One variable left, so we've either book or verse.
  if (book_was_given)
  {
    // Book was given, so it is a chapter.
    ustring ch = number_in_string (input[0]);
    newverse = "1";
    if (ch.empty ())
    {
      return false;
    }
    newchapter = convert_to_int (ch);
    return true;
  }
  else
  {
    // It is a verse.
    newverse = lowerCase (input[0]);
    newchapter = oldchapter;
    if (newverse.empty ())
    {
      return false;
    }
    return true;
  }
}


bool reference_discover (unsigned int oldbook, unsigned int oldchapter, 
                         const ustring& oldverse, const ustring& reference,
                         unsigned int& newbook, unsigned int& newchapter, ustring& newverse)
{
/* 
This is the new function "reference_discover". It uses the previous one which
has now been renamed "reference_discover_internal".
This new function iterates even more over a references, and is able to cut
off bits at the beginning that would not be a references. This occurs when 
loading a file with references saved by BibleWorks. It has a format as 
shown here:
 
BWRL 1

KJV 2Ki 25:18
KJV 1Ch 6:36

In this example the "KJV" needs to be taken out and then the reference will 
appear cleanly.
*/
  // Do the discovery.
  bool result;
  result = reference_discover_internal (oldbook, oldchapter, oldverse, reference, newbook, newchapter, newverse);
  if (!result) {
    if (reference.length() >= 11) {
      ustring adaptedreference (reference);
      adaptedreference.erase (0, 4);
      result = reference_discover_internal (oldbook, oldchapter, oldverse, adaptedreference, newbook, newchapter, newverse);
    }
  }
  return result;
}


unsigned int book_find_valid_internal (const ustring & rawbook)
// This looks whether "rawbook" can be interpreted as a valid book in any way.
{
  
  unsigned int index;
  // Check on names entered like Genesis or 1 Corinthians, the full English name
  // A bug was discovered so that "Judges" was interpreted as "Jude", because
  // of the three letters "JUD". Solved by checking on full English name first.
  // This also is faster, as most books come in with their normal English names.
  index = books_english_to_id (rawbook);
  if (index) {
    return index;
  }
  // Recognise the abbreviations used by Paratext.
  index = books_paratext_to_id (rawbook);
  if (index) {
    return index;
  }
  // Try the abbreviations defined by the OSIS project.
  index = books_osis_to_id (rawbook);
  if (index) {
    return index;
  }
  // Try the abbreviations of BibleWorks.
  index = books_bibleworks_to_id (rawbook);
  if (index) {
    return index;
  }
  // Abbreviations in Paratext: See if shortening the bookname helps.
  if (rawbook.length () >= 3) {
    ustring abbrev = rawbook.substr (0, 3);
    index = books_paratext_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // BibleWorks: See if shortening the bookname helps.
  if (rawbook.length () >= 3) {
    ustring abbrev = rawbook.substr (0, 3);
    index = books_bibleworks_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // OSIS. See if shortening the bookname helps.
  // The shortest abbreviation is 2 characters long,
  // and the longest 6. So we've to try them all.
  // Length: 2
  if (rawbook.length () >= 2) {
    ustring abbrev = rawbook.substr (0, 2);
    index = books_osis_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // Length: 3
  if (rawbook.length () >= 3) {
    ustring abbrev = rawbook.substr (0, 3);
    index = books_osis_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // Length: 4
  if (rawbook.length () >= 4) {
    ustring abbrev = rawbook.substr (0, 4);
    index = books_osis_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // Length: 5
  if (rawbook.length () >= 5) {
    ustring abbrev = rawbook.substr (0, 5);
    index = books_osis_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // Length: 6
  if (rawbook.length () >= 6) {
    ustring abbrev = rawbook.substr (0, 6);
    index = books_osis_to_id (abbrev);
    if (index) {
      return index;
    }
  }
  // The book has not yet been found.
  // Not found yet, check on names like "1Corinthians".
  if (rawbook.length () >= 1) {
    ustring s = rawbook.substr (0, 1);
    ustring s2 = rawbook;
    if (s == "1" || s == "2" || s == "3")
      s2.insert (1, " ");
    index = books_english_to_id (s2);
    if (index) {
      return index;
    }
  }
  // Not yet found. 
  // Go through the language of the project, and see if the book is among the 
  // booknames or abbreviations.
  {
    extern Settings * settings;
    ProjectConfiguration * projectconfig = settings->projectconfig (settings->genconfig.project_get());
    index = books_name_to_id (projectconfig->language_get(), rawbook);
    if (index) return index;
    index = books_abbreviation_to_id (projectconfig->language_get(), rawbook);
    if (index) return index;
    index = books_abbreviation_to_id_loose (projectconfig->language_get(), rawbook);
    if (index) return index;
  }
  // Still not found.
  // Go through all available languages, and see if the book is among the
  // names or abbreviations of the book.
  {
    extern BookLocalizations * booklocalizations;
    vector<ustring> languages = booklocalizations->localizations_get ();
    for (unsigned int i = 0; i < languages.size(); i++) {
      index = books_name_to_id (languages[i], rawbook);
      if (index) return index;
      index = books_abbreviation_to_id (languages[i], rawbook);
      if (index) return index;
      index = books_abbreviation_to_id_loose (languages[i], rawbook);
      if (index) return index;
    }
  }
  return 0;
}


unsigned int book_find_valid (const ustring & rawbook)
// Returns the id of the raw book. Returns 0 if no book was found.
{
  // It uses a mechanism to speed up discovery of the book.
  // We keep a list of raw books in memory, 
  // and a list of the ids that have been assigned to those raw books.
  // If the same raw book is passed again, we just return the previously 
  // discovered id.
  static vector<unsigned int> assigned_ids;
  static vector<ustring> raw_books;
  for (unsigned int i = 0; i < raw_books.size(); i++) {
    if (rawbook == raw_books[i]) {
      return assigned_ids[i];
    }
  }
  int id = book_find_valid_internal (rawbook);
  assigned_ids.push_back (id);
  raw_books.push_back (rawbook);
  return id;
}


unsigned int reference_to_numerical_equivalent (const ustring& book, const ustring& chapter, const ustring& verse)
/*
Produces the numerical equivalent of a reference.
Supports half verses, like 10a, and 11b.
Genesis 1:1 becomes 1001002
Genesis 1:2 becomes 1001004
Exodus  2:1 becomes 2001002
Etc.
*/
{
  unsigned int i;
  i = books_english_to_id (book) * 1000000;
  i = i + (convert_to_int (chapter) * 1000);
  vector<int> verses = verses_encode (verse);
  i = i + verses[0];
  return i;
}


unsigned int reference_to_numerical_equivalent (const Reference& reference)
/*
Produces the numerical equivalent of a reference.
Supports half verses, like 10a, and 11b.
Genesis 1:1 becomes 1001002
Genesis 1:2 becomes 1001004
Exodus  2:1 becomes 2001002
Etc.
*/
{
  unsigned int i;
  i = reference.book * 1000000;
  i = i + (reference.chapter * 1000);
  vector<int> verses = verses_encode (reference.verse);
  i = i + verses[0];
  return i;
}


ustring book_chapter_verse_to_reference (const ustring& book, int chapter, const ustring& verse)
/*
Changes a bookname, with a chapter number, and a verse number, 
to a full references, e.g. "Genesis 1:1a-4".
*/
{
  ustring reference (book);
  reference.append (" ");
  reference.append (convert_to_string(chapter));
  reference.append (":");
  reference.append (verse);
  return reference;
}


bool looks_like_verse (const ustring& text)
// This checks the text given and sees whether it looks like a verse. If so
// it returns true.
{
  // If it is a number only, it looks like a verse.
  if (number_in_string (text) == text)
    return true;
  // If it contains a hyphen (-) or a comma (,) it is a verse number.
  if (text.find_first_of (",-") != string::npos)
    return true;
  // If it contains a digit followed by either an "a" or a "b", it's a verse.
  // Note everything is capitalized, so we check for "A" or "B".
  if (unix_fnmatch ("*[0-9][A,B]*", text))
    return true;
  return false;
}


void verses_encode_internal (const ustring& verse, vector<int>& expanded_verses)
{
  int expanded_verse;
  expanded_verse = 2 * (convert_to_int (verse));
  if (verse.find_first_of ("aA") != string::npos) {
    expanded_verses.push_back (expanded_verse);
  } else if (verse.find_first_of ("bB") != string::npos) {
    expanded_verses.push_back (++expanded_verse);
  } else {
    expanded_verses.push_back (expanded_verse);
    expanded_verses.push_back (++expanded_verse);
  }
}


vector<int> verses_encode (const ustring& verse)
/*
This encodes a verse into a number of integers. As we may have ranges of verses,
like 1b-5, or 1b,2, we handle these ranges or sequences by converting them to
a series of integers values, each integer value representing half of a verse.
So verse 0 becomes then "0, 1", and verse 1 will be "2, 3". Verse 1a will be 
"2".
*/
{
  // Storage.
  vector<int> expanded_verses;
  // Work on a copy of the verse;
  ustring vs = verse;
  // If there is a range, take the beginning and the end and fill up in between.
  if (vs.find ("-") != string::npos) {
    size_t position;
    position = vs.find ("-");
    ustring start_range, end_range;
    start_range = vs.substr (0, position);
    vs.erase (0, ++position);
    end_range = vs;
    int start_expanded_verse = 2 * convert_to_int (number_in_string (start_range));
    if (start_range.find_first_of ("bB") != string::npos)
      start_expanded_verse++;
    int end_expanded_verse = 2 * convert_to_int (number_in_string (end_range));
    if (end_range.find_first_of ("aA") == string::npos)
      end_expanded_verse++;
    // Sometimes people give start higher than the end, so swap them here.
    {
      int min = MIN (start_expanded_verse, end_expanded_verse);
      int max = MAX (start_expanded_verse, end_expanded_verse);
      start_expanded_verse = min;
      end_expanded_verse = max;
    }
    for (int i2 = start_expanded_verse; i2 <= end_expanded_verse; i2++) {
      expanded_verses.push_back (i2);
    }
  } 
  // If there is a sequence, take each verse in the sequence, and store it.
  else if (vs.find (",") != string::npos) {
    int iterations = 0;
    do {
      // In case of an unusual range formation, do not hang, but give message.
      iterations++;
      if (iterations > 50) {
        break;
      }
      size_t position = vs.find (",");
      ustring localverse;
      if (position == string::npos) {
        localverse = vs;
        vs.clear();
      } else {
        localverse = vs.substr (0, position);
        vs.erase (0, ++position);
      }
      verses_encode_internal (localverse, expanded_verses);
    } while (!vs.empty());
  }
  // No range and no sequence: a "normal" verse.
  else {
    verses_encode_internal (vs, expanded_verses);
  }
  // Return result.
  return expanded_verses;
}


bool chapter_span_discover (const ustring& reference, ustring& chapter1, ustring& verse1, ustring& chapter2, ustring& verse2)
// Discover whether the reference spans the chapter boundary.
// E.g. Luke 1:3-2.5
{
  // Work on a copy of the reference.
  ustring ref_in (reference);
  
  // Change colons to spaced dots, space hyphens, and parse it into the separate words.
  replace_text (ref_in, ".", " . ");
  replace_text (ref_in, ":", " . ");
  replace_text (ref_in, "-", " - ");
  Parse parse (ref_in, false);

  // Is the chapter spanning signature (two dots, one hyphen) in it?
  int hyphencount = 0;
  int dotcount = 0;
  for (unsigned int i = 0; i < parse.words.size(); i++) {
    if (parse.words[i] == "-") hyphencount++;
    if (parse.words[i] == ".") dotcount++;
  }
  if (hyphencount == 0) return false;
  if (dotcount < 2) return false;

  // There might be chapter spanning at this point.
  // Are the dots and hyphens in the right order? Should be: dot, hyphen, dot.
  int firstdotlocation = -1;
  int seconddotlocation = -1;
  int hyphenlocation = -1;
  for (unsigned int i = 0; i < parse.words.size(); i++) {
    if (parse.words[i] == ".") {
      if (firstdotlocation == -1) {
        firstdotlocation = i;
      } else {
        seconddotlocation = i;
      }
    }
    if (parse.words[i] == "-") {
      hyphenlocation = i;
    }
  }
  if (firstdotlocation > hyphenlocation) return false;
  if (hyphenlocation > seconddotlocation) return false;
  
  // We'll take it that there is chapter spanning at this point.
  // Let's extract the chapter / verse boundaries.
  if (firstdotlocation > 0) 
    chapter1 = parse.words[firstdotlocation - 1];
  verse1 = parse.words[hyphenlocation - 1];
  chapter2 = parse.words[seconddotlocation - 1];
  if ((unsigned int) (seconddotlocation + 1) < parse.words.size()) 
    verse2 = parse.words[seconddotlocation + 1];

  // Do we have all information?
  if (chapter1.empty()) return false;
  if (verse1.empty()) return false;
  if (chapter2.empty()) return false;
  if (verse2.empty()) return false;
  
  // Right then, it spans the chapter boundary.
  return true;
}


vector <unsigned int> verse_range_sequence (const ustring& verse)
// Returns the verse(s) that are in variable "verse".
// It handles a single verse, a range of verses, or a sequence of verses.
// Examples:
// 10
// 10-12
// 10,11


// Note: Do not improve this function anymore, but instead use verse_range_sequence_v2


{
  // Result.
  vector <unsigned int> verses;
  
  // Work on a copy.
  ustring vs (verse);

  // If there is a range, take the beginning and the end and fill up in between.
  if (vs.find ("-") != string::npos) {
    size_t position;
    position = vs.find ("-");
    ustring start_range, end_range;
    start_range = vs.substr (0, position);
    vs.erase (0, ++position);
    end_range = vs;
    unsigned int start_verse_i = convert_to_int (number_in_string (start_range));
    unsigned int end_verse_i = convert_to_int (number_in_string (end_range));
    for (unsigned int i = start_verse_i; i <= end_verse_i; i++) {
      verses.push_back (i);
    }
  } 

  // Else if there is a sequence, take each verse in the sequence, and store it.
  else if (vs.find (",") != string::npos) {
    int iterations = 0;
    do {
      // In case of an unusual range formation, do not hang, but give message.
      iterations++;
      if (iterations > 50) {
        break;
      }
      size_t position = vs.find (",");
      ustring verse;
      if (position == string::npos) {
        verse = vs;
        vs.clear();
      } else {
        verse = vs.substr (0, position);
        vs.erase (0, ++position);
      }
      verses.push_back (convert_to_int (verse));
    } while (!vs.empty());
  }
  
  // No range and no sequence: a "normal" verse.
  else {
    verses.push_back (convert_to_int (vs));
  }
  
  // Return the verses we got.
  return verses;
}


vector <ustring> verse_range_sequence_v2 (ustring verse)
// Returns the verse(s) that are in variable "verse".
// It handles a single verse, a range of verses, or a sequence of verses.
// Examples:
// 10
// 10-12b
// 10,11a
// 10,12
{
  // Result.
  vector <ustring> verses;
  
  // If there is a range, take the beginning and the end and fill up in between.
  if (verse.find ("-") != string::npos) {
    size_t position;
    position = verse.find ("-");
    ustring start_range, end_range;
    start_range = verse.substr (0, position);
    verse.erase (0, ++position);
    end_range = verse;
    unsigned int start_verse_i = convert_to_int (number_in_string (start_range));
    unsigned int end_verse_i = convert_to_int (number_in_string (end_range));
    for (unsigned int i = start_verse_i; i <= end_verse_i; i++) {
      if (i == start_verse_i) verses.push_back (start_range);
      else if (i == end_verse_i) verses.push_back (end_range);
      else verses.push_back (convert_to_string (i));
    }
  } 

  // Else if there is a sequence, take each verse in the sequence, and store it.
  else if (verse.find (",") != string::npos) {
    int iterations = 0;
    do {
      // In case of an unusual range formation, do not hang, but give message.
      iterations++;
      if (iterations > 50) {
        break;
      }
      size_t position = verse.find (",");
      ustring vs;
      if (position == string::npos) {
        vs = verse;
        verse.clear();
      } else {
        vs = verse.substr (0, position);
        verse.erase (0, ++position);
      }
      verses.push_back (vs);
    } while (!verse.empty());
  }
  
  // No range and no sequence: a "normal" verse.
  else {
    verses.push_back (verse);
  }
  
  // Return the verses we got.
  return verses;
}

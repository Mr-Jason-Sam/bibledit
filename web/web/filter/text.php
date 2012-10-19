<?php


/**
* @package bibledit
*/
/*
 ** Copyright (©) 2003-2012 Teus Benschop.
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
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */


/**
* This class filters USFM text, converting it into other formats.
*/
class Filter_Text
{
  
  private $usfmMarkersAndText; // Array of strings alternating between USFM and text.
  private $usfmMarkersAndTextPointer;

  private $chapterUsfmMarkersAndText; // Array holding a chapter of USFM code, alternating between USFM and text.
  private $chapterUsfmMarkersAndTextPointer;
  
  private $styles; // An array holding arrays of style information.
  private $chapterMarker; // Usually this is: c
  private $createdOdfStyles; // Array holding styles created in Odf_Text class.
  
  private $currentBookIdentifier; // Book identifier, e.g. 1, 2, 3, and so on.
  private $currentChapterNumber; // Chapter number, e.g. 1, 2, 3, etc.
  private $currentVerseNumber; // Verse number, e.g. "0", "1", "2", and so on.
  public $numberOfChaptersPerBook; // Array of (book, chapter number).
  
  public $runningHeaders; // Array with numerical keys, and values like array (book, chapter, verse, marker, header value).
  public $longTOCs; // Array with numerical keys, and values like array (book, chapter, verse, marker, TOC value).
  public $shortTOCs; // Array with numerical keys, and values like array (book, chapter, verse, marker, TOC value).
  public $bookAbbreviations; // Array with numerical keys, and values like array (book, chapter, verse, marker, abbreviation value).

  private $outputChapterTextAtFirstVerse; // String holding the chapter number or text to output at the first verse.
  public $chapterLabels; // Array with numerical keys, and values like array (book, chapter, verse, marker, label value).
  public $publishedChapterMarkers; // Array with numerical keys, and values like array (book, chapter, verse, marker, marker value).

  public $odf_text_standard; // Object for creating OpenDocument with text in standard form.
  public $odf_text_text_only; // Object for creating OpenDocument with text only.
  public $odf_text_text_and_note_citations; // Object for creating OpenDocument with text and note citations.
  public $odf_text_notes; // Object for creating OpenDocument with the notes only.

  public $info; // Array with strings.
  public $fallout; // Array with strings.
  private $wordListGlossaryDictionary; // Array with strings.
  private $hebrewWordList; // Array with strings.
  private $greekWordList; // Array with strings.
  private $subjectIndex; // Array with strings.
  
  private $notecitations; // Array with information for the citations for the notes.
  private $standardContentMarkerFootEndNote;
  private $standardContentMarkerCrossReference;
  
  public $html_text_standard; // Object for creating Web document with text in standard form.
  public $html_text_linked; // Object for creating Linked web document.
  
  public $onlinebible_text; // Object for creating the input file for the Online Bible compiler.
  
  public $esword_text; // Object for creating the Bible module for eSword. Todo
  
  /**
  * Class constructor.
  */
  public function __construct ($bible)
  {
    $this->createdOdfStyles = array ();
    $this->numberOfChaptersPerBook = array ();
    $this->runningHeaders = array ();
    $this->longTOCs = array ();
    $this->shortTOCs = array ();
    $this->bookAbbreviations = array ();
    $this->chapterLabels = array ();
    $this->publishedChapterMarkers = array ();
    $this->odf_text_standard = new Odf_Text;
    $this->odf_text_text_only = new Odf_Text;
    $this->odf_text_text_and_note_citations = new Odf_Text;
    $this->odf_text_notes = new Odf_Text;
    $this->info = array ();
    $this->fallout = array ();
    $this->wordListGlossaryDictionary = array ();
    $this->hebrewWordList = array ();
    $this->greekWordList = array ();
    $this->subjectIndex = array ();
    $this->notecitations = array ();
    $this->standardContentMarkerFootEndNote = "";
    $this->standardContentMarkerCrossReference = "";
    $this->html_text_standard = new Html_Text (gettext ("Bible"));
    $this->html_text_linked = new Html_Text (gettext ("Bible"));
    $this->onlinebible_text = new Onlinebible_Text ();
    $this->esword_text = new Esword_Text ($bible);
  }
  

  /**
  * This function adds USFM code to the class.
  * $code: string or array of USFM code.
  */
  public function addUsfmCode($code)
  {
    // Get the USFM $code as a string.
    if (is_array ($code)) {
      $code = implode ("\n", $code);
    }
    $code .= "\n";
    // Sort the USFM $code out and separate it into markers and text.
    $markersAndText = Filter_Usfm::getMarkersAndText ($code);
    foreach ($markersAndText as $item) {
      $this->usfmMarkersAndText [] = $item;
    }
  }
  
  
  /**
  * This function runs the filter.
  * $stylesheet - The stylesheet to use.
  */
  public function run ($stylesheet)
  {
    // Get the styles.
    $this->getStyles ($stylesheet);

    // Preprocess.
    $this->preprocessingStage ();

    // Process data.
    $this->processUsfm ();
  }


  /**
  * This function return true when there is still unprocessed USFM code available.
  */
  private function unprocessedUsfmCodeAvailable ()
  {
    return (($this->usfmMarkersAndTextPointer) < count ($this->usfmMarkersAndText));
  }
  
  
  /**
  * This function stores data in the class: 
  * the next chapter from the unprocessed USFM code.
  */
  private function getUsfmNextChapter ()
  {
    // Initialization.
    $this->chapterUsfmMarkersAndText = array ();
    $this->chapterUsfmMarkersAndTextPointer = 0;
    $firstLine = true;
    
    // Obtain the standard marker for the chapter number.
    // Deal with the unlikely case that the chapter marker is non-standard.
    if (!isset ($this->chapterMarker)) {
      $this->chapterMarker = "c";
      foreach ($this->styles as $style) {
        if ($style["type"] == StyleTypeChapterNumber) {
          $this->chapterMarker = $style["marker"];
          break;
        }
      }
    }

    while ($this->unprocessedUsfmCodeAvailable ()) {
      $item = $this->usfmMarkersAndText[$this->usfmMarkersAndTextPointer];
      if (!$firstLine) {
        if (trim ($item) == ("\\" . $this->chapterMarker)) {
          return;
        }
      }
      $this->chapterUsfmMarkersAndText [] = $item;
      $firstLine = false;
      $this->usfmMarkersAndTextPointer++;
    }
  }
  

  /**
  * This function gets the styles from the database, 
  * and stores them in the object for quicker access.
  */
  private function getStyles ($stylesheet)
  {
    $this->styles = array ();
    $styles_logic = Styles_Logic::getInstance (); // This is to get the relevant styles information included.
    $this->odf_text_standard->createPageBreakStyle ();
    $this->odf_text_text_only->createPageBreakStyle ();
    $this->odf_text_text_and_note_citations->createPageBreakStyle ();
    $this->odf_text_text_and_note_citations->createSuperscriptStyle ();
    $database_styles = Database_Styles::getInstance ();
    $markers = $database_styles->getMarkers ($stylesheet);
    foreach ($markers as $marker) {
      $style = $database_styles->getMarkerData ($stylesheet, $marker);
      $this->styles [$marker] = $style;
      if ($style['type'] == StyleTypeFootEndNote) {
        if ($style['subtype'] == FootEndNoteSubtypeStandardContent) {
          $this->standardContentMarkerFootEndNote = $style['marker'];
        }
      }
      if ($style['type'] == StyleTypeCrossreference) {
        if ($style['subtype'] == CrossreferenceSubtypeStandardContent) {
          $this->standardContentMarkerCrossReference = $style['marker'];
        }
      }
    }
  }



  /**
  * This function does the preprocessing of the USFM code 
  * extracting a variety of information, creating note citations, etc.
  */
  private function preprocessingStage ()
  {
    $this->usfmMarkersAndTextPointer = 0;
    while ($this->unprocessedUsfmCodeAvailable ()) {
      $this->getUsfmNextChapter ();
      for ($this->chapterUsfmMarkersAndTextPointer = 0; $this->chapterUsfmMarkersAndTextPointer < count ($this->chapterUsfmMarkersAndText); $this->chapterUsfmMarkersAndTextPointer++) {
        $currentItem = $this->chapterUsfmMarkersAndText[$this->chapterUsfmMarkersAndTextPointer];
        if (Filter_Usfm::isUsfmMarker ($currentItem)) {
          $marker = trim ($currentItem); // Change, e.g. '\id ' to '\id'.
          $marker = substr ($marker, 1); // Remove the initial backslash, e.g. '\id' becomes 'id'.
          if (Filter_Usfm::isOpeningMarker ($marker)) {
            if (array_key_exists ($marker, $this->styles)) {
              $style = $this->styles[$marker];
              switch ($style['type']) {
                case StyleTypeIdentifier:
                  switch ($style['subtype']) {
                    case IdentifierSubtypeBook:
                    {
                      // Get book number.
                      $s = Filter_Usfm::getBookIdentifier ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $s = str_replace (Filter_Character::softHyphen (), "", $s); // Remove possible soft hyphen.
                      $database_books = Database_Books::getInstance ();
                      $this->currentBookIdentifier = $database_books->getIdFromUsfm ($s);
                      // Reset chapter and verse numbers.
                      $this->currentChapterNumber = 0;
                      $this->numberOfChaptersPerBook[$this->currentBookIdentifier] = 0;
                      $this->currentVerseNumber = "0";
                      break;
                    }
                    case IdentifierSubtypeRunningHeader:
                    {
                      $runningHeader = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $this->runningHeaders [] = array ('book' => $this->currentBookIdentifier, 'chapter' => $this->currentChapterNumber, 'verse' => $this->currentVerseNumber, 'marker' => $marker, 'value' => $runningHeader);
                      break;
                    }
                    case IdentifierSubtypeLongTOC:
                    {
                      $longTOC = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $this->longTOCs [] = array ('book' => $this->currentBookIdentifier, 'chapter' => $this->currentChapterNumber, 'verse' => $this->currentVerseNumber, 'marker' => $marker, 'value' => $longTOC);
                      break;
                    }
                    case IdentifierSubtypeShortTOC:
                    {
                      $shortTOC = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $this->shortTOCs [] = array ('book' => $this->currentBookIdentifier, 'chapter' => $this->currentChapterNumber, 'verse' => $this->currentVerseNumber, 'marker' => $marker, 'value' => $shortTOC);
                      break;
                    }
                    case IdentifierSubtypeBookAbbrev:
                    {
                      $bookAbbreviation = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $this->bookAbbreviations [] = array ('book' => $this->currentBookIdentifier, 'chapter' => $this->currentChapterNumber, 'verse' => $this->currentVerseNumber, 'marker' => $marker, 'value' => $bookAbbreviation);
                      break;
                    }
                    case IdentifierSubtypeChapterLabel:
                    {
                      $chapterLabel = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $this->chapterLabels [] = array ('book' => $this->currentBookIdentifier, 'chapter' => $this->currentChapterNumber, 'verse' => $this->currentVerseNumber, 'marker' => $marker, 'value' => $chapterLabel);
                      break;
                    }
                    case IdentifierSubtypePublishedChapterMarker:
                    {
                      $publishedChapterMarker = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                      $this->publishedChapterMarkers [] = array ('book' => $this->currentBookIdentifier, 'chapter' => $this->currentChapterNumber, 'verse' => $this->currentVerseNumber, 'marker' => $marker, 'value' => $publishedChapterMarker);
                      break;
                    }
                  }
                  break;
                case StyleTypeChapterNumber:
                {
                  $number = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                  $number = Filter_Numeric::integer_in_string ($number);
                  $this->currentChapterNumber = $number;
                  $this->numberOfChaptersPerBook[$this->currentBookIdentifier] = $number;
                  $this->currentVerseNumber = "0";
                  break;
                }
                case StyleTypeVerseNumber:
                {
                  $number = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                  $number = Filter_Numeric::integer_in_string ($number);
                  $this->currentVerseNumber = $number;
                  break;
                }
                case StyleTypeFootEndNote: 
                {
                  switch ($style['subtype']) 
                  {
                    case FootEndNoteSubtypeFootnote:
                    case FootEndNoteSubtypeEndnote:
                    {
                      $this->createNoteCitation ($style);
                      break;
                    }
                    case FootEndNoteSubtypeStandardContent:
                    case FootEndNoteSubtypeContent:
                    case FootEndNoteSubtypeContentWithEndmarker:
                    case FootEndNoteSubtypeParagraph:
                    {
                      break;
                    }
                  }
                  break;
                }
                case StyleTypeCrossreference:
                {
                  switch ($style['subtype']) 
                  {
                    case CrossreferenceSubtypeCrossreference:
                    {
                      $this->createNoteCitation ($style);
                      break;
                    }
                    case CrossreferenceSubtypeStandardContent:
                    case CrossreferenceSubtypeContent:
                    case CrossreferenceSubtypeContentWithEndmarker:
                    {
                      break;
                    }
                  }
                  break;
                }
              }
            }
          }
        }
      }
    }
  }


  /**
  * This function does the processing of the USFM code,
  * formatting the document and extracting other useful information.
  */
  private function processUsfm ()
  {
    // The linked web file starts with a link back to the index.
    $this->html_text_linked->newParagraph ();
    $this->html_text_linked->addLink ($this->html_text_linked->currentPDomElement,  "index.html", "", gettext ("Index"), "", gettext ("Index"));

    // Go through the USFM code.
    $processedBooksCount = 0;
    $this->usfmMarkersAndTextPointer = 0;
    while ($this->unprocessedUsfmCodeAvailable ()) {
      $this->getUsfmNextChapter ();
      for ($this->chapterUsfmMarkersAndTextPointer = 0; $this->chapterUsfmMarkersAndTextPointer < count ($this->chapterUsfmMarkersAndText); $this->chapterUsfmMarkersAndTextPointer++) {
        $currentItem = $this->chapterUsfmMarkersAndText[$this->chapterUsfmMarkersAndTextPointer];
        if (Filter_Usfm::isUsfmMarker ($currentItem)) 
        {
          // Store indicator whether the marker is an opening marker.
          $isOpeningMarker = Filter_Usfm::isOpeningMarker ($currentItem);
          // Clean up the marker, so we remain with the basic version, e.g. 'id'.
          $marker = Filter_Usfm::getMarker ($currentItem);
          if (array_key_exists ($marker, $this->styles)) 
          {
            $style = $this->styles[$marker];
            switch ($style['type']) 
            {
              case StyleTypeIdentifier:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                switch ($style['subtype']) 
                {
                  case IdentifierSubtypeBook:
                  {
                    // Get book number.
                    $s = Filter_Usfm::getBookIdentifier ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    $s = str_replace (Filter_Character::softHyphen (), "", $s); // Remove possible soft hyphen.
                    $database_books = Database_Books::getInstance ();
                    $this->currentBookIdentifier = $database_books->getIdFromUsfm ($s);
                    // Reset chapter and verse numbers.
                    $this->currentChapterNumber = 0;
                    $this->currentVerseNumber = "0";
                    // Throw away whatever follows the \id, e.g. 'GEN xxx xxx'.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    // Whether to insert a new page before the book. But never before the first book.
                    if ($style['userbool1']) {
                      if ($processedBooksCount) {
                        $this->odf_text_standard->newPageBreak ();
                        $this->odf_text_text_only->newPageBreak ();
                        $this->odf_text_text_and_note_citations->newPageBreak ();
                        $this->html_text_standard->newPageBreak (); // Todo
                        $this->html_text_linked->newPageBreak ();
                      }
                    }
                    $processedBooksCount++;
                    // Reset notes.
                    $this->resetNoteCitations ('book');
                    // Online Bible.
                    $this->onlinebible_text->storeData ();
                    // Linked web: Links to all the chapters, and back to the start.
                    $this->html_text_linked->newParagraph ();
                    $this->html_text_linked->addLink ($this->html_text_linked->currentPDomElement, "", "start", "", "", "");
                    $this->html_text_linked->addText (gettext ("Chapter") . " |");
                    for ($i = 1; $i <= $this->numberOfChaptersPerBook[$this->currentBookIdentifier]; $i++) {
                      $this->html_text_linked->addLink ($this->html_text_linked->currentPDomElement, "#chapter$i", "", "Chapter $i", "", " $i |");
                    }
                    // Done.
                    break;
                  }
                  case IdentifierSubtypeEncoding:
                  {
                    $this->addToFallout ("Text encoding indicator not supported. Encoding is always in UTF8: \\$marker", true);
                    break;
                  }
                  case IdentifierSubtypeComment:
                  {
                    $this->addToInfo ("Comment: \\$marker", true);
                    break;
                  }
                  case IdentifierSubtypeRunningHeader:
                  {
                    // This information already went into the Info document during the preprocessing stage.
                    // Remove it from the USFM input stream.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    // Ideally this information should be inserted in the headers of the standard text document.
                    // UserBool2RunningHeaderLeft:
                    // UserBool3RunningHeaderRight:
                    break;
                  }
                  case IdentifierSubtypeLongTOC:
                  {
                    // This information already went into the Info document. Remove it from the USFM stream.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    break;
                  }
                  case IdentifierSubtypeShortTOC:
                  {
                    // This information already went into the Info document. Remove it from the USFM stream.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    break;
                  }
                  case IdentifierSubtypeBookAbbrev:
                  {
                    // This information already went into the Info document. Remove it from the USFM stream.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    break;
                  }
                  case IdentifierSubtypeChapterLabel:
                  {
                    // This information is already in the object. Remove it from the USFM stream.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    break;
                  }
                  case IdentifierSubtypePublishedChapterMarker:
                  {
                    // This information is already in the object. Remove it from the USFM stream.
                    Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                    break;
                  }
                  case IdentifierSubtypeCommentWithEndmarker:
                  {
                    if ($isOpeningMarker) {
                      $this->addToInfo ("Comment: \\$marker", true);
                    }
                    break;
                  }
                  default:
                  {
                    $this->addToFallout ("Unknown markup: \\$marker", true);
                    break;
                  }
                }
                break;
              }
              case StyleTypeNotUsedComment:
              {
                $this->addToFallout ("Unknown markup: \\$marker", true);
                break;
              }
              case StyleTypeNotUsedRunningHeader:
              {
                $this->addToFallout ("Unknown markup: \\$marker", true);
                break;
              }
              case StyleTypeStartsParagraph:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                switch ($style['subtype']) 
                {
                  case ParagraphSubtypeMainTitle:
                  case ParagraphSubtypeSubTitle:
                  case ParagraphSubtypeSectionHeading:
                  {
                    $this->newParagraph ($style, true);
                    break;
                  }
                  case ParagraphSubtypeNormalParagraph:
                  default:
                  {
                    $this->newParagraph ($style, false);
                    break;
                  }
                }
                break;
              }
              case StyleTypeInlineText:
              {
                if ($isOpeningMarker) {
                  $this->odf_text_standard->openTextStyle ($style);
                  $this->odf_text_text_only->openTextStyle ($style);
                  $this->odf_text_text_and_note_citations->openTextStyle ($style);
                  $this->html_text_standard->openTextStyle ($style); // Todo
                  $this->html_text_linked->openTextStyle ($style);
                } else {
                  $this->odf_text_standard->closeTextStyle ();
                  $this->odf_text_text_only->closeTextStyle ();
                  $this->odf_text_text_and_note_citations->closeTextStyle ();
                  $this->html_text_standard->closeTextStyle (); // Todo
                  $this->html_text_linked->closeTextStyle ();
                }
                break;
              }
              case StyleTypeChapterNumber:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                
                $this->onlinebible_text->storeData ();

                // Get the chapter number.
                $number = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                $number = Filter_Numeric::integer_in_string ($number);

                // Update this object.
                $this->currentChapterNumber = $number;
                $this->currentVerseNumber = "0";

                // If there is a published chapter character, the chapter number takes that value.
                foreach ($this->publishedChapterMarkers as $publishedChapterMarker) {
                  if ($publishedChapterMarker['book'] == $this->currentBookIdentifier) {
                    if ($publishedChapterMarker['chapter'] == $this->currentChapterNumber) {
                      $number = $publishedChapterMarker['value'];
                    }
                  }
                }

                // Enter text for the running headers.
                $database_books = Database_Books::getInstance ();
                $runningHeader = $database_books->getEnglishFromId ($this->currentBookIdentifier);
                foreach ($this->runningHeaders as $item) {
                  if ($item['book'] == $this->currentBookIdentifier) {
                    $runningHeader = $item['value'];
                  }
                }
                $runningHeader = "$runningHeader $number";
                $this->odf_text_standard->newHeading1 ($runningHeader, true);
                $this->odf_text_text_only->newHeading1 ($runningHeader, true);
                $this->odf_text_text_and_note_citations->newHeading1 ($runningHeader, true);
                $this->odf_text_notes->newHeading1 ($runningHeader, false);
                
                // In the linked web version, each chapter number gets linked to, and links to, the start of the book.
                $this->html_text_linked->newParagraph ();
                $hrefId = "chapter" . $this->currentChapterNumber;
                $this->html_text_linked->addLink ($this->html_text_linked->currentPDomElement, "#start", $hrefId, gettext ("Go to start"), "", gettext ("Go to start"));

                // This is the phase of outputting the chapter number in the text body. 
                // The chapter number is only output when there is more than one chapter in a book.
                if ($this->numberOfChaptersPerBook[$this->currentBookIdentifier] > 1) {
                  if ($style['userbool1']) {
                    // Output the chapter number at the first verse, not here.
                    // Store it for later processing.
                    $this->outputChapterTextAtFirstVerse = $number;
                  } else {
                    // Output the chapter in a new paragraph.
                    // If the chapter label \cl is entered once before chapter 1 (\c 1) 
                    // it represents the text for "chapter" to be used throughout the current book. 
                    // If \cl is used after each individual chapter marker, it represents the particular text 
                    // to be used for the display of the current chapter heading 
                    // (usually done if numbers are being presented as words, not numerals).
                    $labelEntireBook = "";
                    $labelCurrentChapter = "";
                    foreach ($this->chapterLabels as $pchapterLabel) {
                      if ($pchapterLabel['book'] == $this->currentBookIdentifier) {
                        if ($pchapterLabel['chapter'] == 0) {
                          $labelEntireBook = $pchapterLabel['value'];
                        }
                        if ($pchapterLabel['chapter'] == $this->currentChapterNumber) {
                          $labelCurrentChapter = $pchapterLabel['value'];
                        }
                      }
                    }
                    if ($labelEntireBook != "") {
                      $number = "$labelEntireBook $number";
                    } 
                    if ($labelCurrentChapter != "") {
                      $number = $labelCurrentChapter;
                    }
                    // The chapter number shows in a new paragraph. 
                    // Keep it together with the next paragraph.
                    $this->newParagraph ($style, true);
                    $this->odf_text_standard->addText ($number);
                    $this->odf_text_text_only->addText ($number);
                    $this->odf_text_text_and_note_citations->addText ($number);
                    $this->html_text_standard->addText ($number); // Todo
                    $this->html_text_linked->addText ($number);
                  }
                }

                // Open a paragraph for the notes. It takes the style of the footnote content marker, usually 'ft'. 
                // This is done specifically for the version that has the notes only.
                $this->ensureNoteParagraphStyle ($this->standardContentMarkerFootEndNote, $this->styles[$this->standardContentMarkerFootEndNote]);
                $this->odf_text_notes->newParagraph ($this->standardContentMarkerFootEndNote);
                // UserBool2ChapterInLeftRunningHeader -> no headings implemented yet.
                // UserBool3ChapterInRightRunningHeader -> no headings implemented yet.

                // Reset.
                $this->resetNoteCitations ('chapter');
                
                // Done.
                break;
              }
              case StyleTypeVerseNumber:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                $this->onlinebible_text->storeData ();
                // Care for the situation that a new verse starts a new paragraph.
                if ($style['userbool1']) {
                  if ($this->odf_text_standard->currentParagraphContent != "") {
                    $this->odf_text_standard->newParagraph ($this->odf_text_standard->currentParagraphStyle);
                  }
                  if ($this->odf_text_text_only->currentParagraphContent != "") {
                    $this->odf_text_text_only->newParagraph ($this->odf_text_text_only->currentParagraphStyle);
                  }
                  if ($this->odf_text_text_and_note_citations->currentParagraphContent != "") {
                    $this->odf_text_text_and_note_citations->newParagraph ($this->odf_text_text_and_note_citations->currentParagraphStyle);
                  }
                  if ($this->html_text_standard->currentParagraphContent != "") { // Todo
                    $this->html_text_standard->newParagraph ($this->html_text_standard->currentParagraphStyle); // Todo
                  }
                  if ($this->html_text_linked->currentParagraphContent != "") {
                    $this->html_text_linked->newParagraph ($this->html_text_linked->currentParagraphStyle);
                  }
                }
                // Deal with the case of a pending chapter number.
                if (isset ($this->outputChapterTextAtFirstVerse)) {
                  $database_config_general = Database_Config_General::getInstance ();
                  if ($database_config_general->getExportChapterDropCaps ()) {
                    $dropCapsLength = mb_strlen ($this->outputChapterTextAtFirstVerse);
                    $this->applyDropCapsToCurrentParagraph ($dropCapsLength);
                    $this->odf_text_standard->addText ($this->outputChapterTextAtFirstVerse);
                    $this->odf_text_text_only->addText ($this->outputChapterTextAtFirstVerse);
                    $this->odf_text_text_and_note_citations->addText ($this->outputChapterTextAtFirstVerse);
                  } else {
                    $this->putChapterNumberInFrame ($this->outputChapterTextAtFirstVerse);
                  }
                  $this->html_text_standard->openTextStyle (array ("marker" => "dropcaps"), false); // Todo
                  $this->html_text_standard->addText ($this->outputChapterTextAtFirstVerse); // Todo
                  $this->html_text_standard->closeTextStyle (); // Todo
                  $this->html_text_linked->openTextStyle (array ("marker" => "dropcaps"), false);
                  $this->html_text_linked->addText ($this->outputChapterTextAtFirstVerse);
                  $this->html_text_linked->closeTextStyle ();
                }
                // Temporarily retrieve the text that follows the \v verse marker.
                $textFollowingMarker = Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
                // Extract the verse number, and store it in the object.
                $number = Filter_Usfm::peekVerseNumber ($textFollowingMarker);
                $this->currentVerseNumber = $number;
                // Output the verse number. But only if no chapter number was put here.
                if (!isset ($this->outputChapterTextAtFirstVerse)) {
                  // If the current paragraph has text already, then insert a space.
                  if ($this->odf_text_standard->currentParagraphContent != "") {
                    $this->odf_text_standard->addText (" ");
                  }
                  if ($this->odf_text_text_only->currentParagraphContent != "") {
                    $this->odf_text_text_only->addText (" ");
                  }
                  if ($this->odf_text_text_and_note_citations->currentParagraphContent != "") {
                    $this->odf_text_text_and_note_citations->addText (" ");
                  }
                  if ($this->html_text_standard->currentParagraphContent != "") { // Todo
                    $this->html_text_standard->addText (" "); // Todo
                  }
                  if ($this->html_text_linked->currentParagraphContent != "") {
                    $this->html_text_linked->addText (" ");
                  }
                  $this->odf_text_standard->openTextStyle ($style);
                  $this->odf_text_text_only->openTextStyle ($style);
                  $this->odf_text_text_and_note_citations->openTextStyle ($style);
                  $this->html_text_standard->openTextStyle ($style); // Todo
                  $this->html_text_linked->openTextStyle ($style);
                  $this->odf_text_standard->addText ($number);
                  $this->odf_text_text_only->addText ($number);
                  $this->odf_text_text_and_note_citations->addText ($number);
                  $this->html_text_standard->addText ($number); // Todo
                  $this->html_text_linked->addText ($number);
                  $this->odf_text_standard->closeTextStyle ();
                  $this->odf_text_text_only->closeTextStyle ();
                  $this->odf_text_text_and_note_citations->closeTextStyle ();
                  $this->html_text_standard->closeTextStyle (); // Todo
                  $this->html_text_linked->closeTextStyle ();
                }
                // If there was any text following the \v marker, remove the verse number, 
                // put the remainder back into the object, and update the pointer.
                if ($textFollowingMarker != "") {
                  $pos = strpos ($textFollowingMarker, $number);
                  if ($pos !== false) {
                    $textFollowingMarker = substr ($textFollowingMarker, $pos + strlen ($number));
                  }
                  // If a chapter number was put, remove any whitespace from the start of the following text.
                  // Remove whitespace from the start of the following text, and replace it with the en space.
                  // This en space is a fixed width space. Having this after the verse number makes things tidier.
                  // But if a chapter number was put, than do not put any space at the start of the following verse.
                  $textFollowingMarker = ltrim ($textFollowingMarker);
                  if (!isset ($this->outputChapterTextAtFirstVerse)) {
                    $textFollowingMarker = Filter_Character::enSpace () . $textFollowingMarker;
                  }
                  $this->chapterUsfmMarkersAndText [$this->chapterUsfmMarkersAndTextPointer] = $textFollowingMarker; 
                  $this->chapterUsfmMarkersAndTextPointer--;
                }
                // Unset the chapter variable, whether it was used or not. This makes it ready for subsequent use. 
                unset ($this->outputChapterTextAtFirstVerse);
                // Online Bible.
                $this->onlinebible_text->newVerse ($this->currentBookIdentifier, $this->currentChapterNumber, $this->currentVerseNumber);
                // Done.
                break;
              }
              case StyleTypeFootEndNote:
              {
                $this->processNote ();
                break;
              }
              case StyleTypeCrossreference:
              {
                $this->processNote ();
                break;
              }
              case StyleTypePeripheral:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                switch ($style['subtype']) 
                {
                  case PeripheralSubtypePublication:
                  case PeripheralSubtypeTableOfContents:
                  case PeripheralSubtypePreface:
                  case PeripheralSubtypeIntroduction:
                  case PeripheralSubtypeGlossary:
                  case PeripheralSubtypeConcordance:
                  case PeripheralSubtypeIndex:
                  case PeripheralSubtypeMapIndex:
                  case PeripheralSubtypeCover:
                  case PeripheralSubtypeSpine:
                  default:
                  {
                    $this->addToFallout ("Unknown pheripheral marker \\$marker", false);
                    break;
                  }
                }
                break;
              }
              case StyleTypePicture:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                $this->addToFallout ("Picture formatting not yet implemented", true);
                break;
              }
              case StyleTypePageBreak:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                $this->odf_text_standard->newPageBreak ();
                $this->odf_text_text_only->newPageBreak ();
                $this->odf_text_text_and_note_citations->newPageBreak ();
                $this->html_text_standard->newPageBreak (); // Todo
                $this->html_text_linked->newPageBreak ();
                break;
              }
              case StyleTypeTableElement:
              {
                $this->odf_text_standard->closeTextStyle ();
                $this->odf_text_text_only->closeTextStyle ();
                $this->odf_text_text_and_note_citations->closeTextStyle ();
                $this->odf_text_notes->closeTextStyle ();
                $this->html_text_standard->closeTextStyle (); // Todo
                $this->html_text_linked->closeTextStyle ();
                switch ($style['subtype']) 
                {
                  case TableElementSubtypeRow:
                  {
                    $this->addToFallout ("Table elements not yet implemented", false);
                    break;
                  }
                  case TableElementSubtypeHeading:
                  case TableElementSubtypeCell:
                  {
                    $this->newParagraph ($style, false);
                    break;
                  }
                  default:
                  {
                    break;
                  }
                }
                // UserInt1TableColumnNumber:
                break;
              }
              case StyleTypeWordlistElement:
              {
                switch ($style['subtype']) 
                {
                  case WorListElementSubtypeWordlistGlossaryDictionary:
                  {
                    if ($isOpeningMarker) {
                      $this->addToWordList ($this->wordListGlossaryDictionary);
                    }
                    break;
                  }
                  case WorListElementSubtypeHebrewWordlistEntry:
                  {
                    if ($isOpeningMarker) {
                      $this->addToWordList ($this->hebrewWordList);
                    }
                    break;
                  }
                  case WorListElementSubtypeGreekWordlistEntry:
                  {
                    if ($isOpeningMarker) {
                      $this->addToWordList ($this->greekWordList);
                    }
                    break;
                  }
                  case WorListElementSubtypeSubjectIndexEntry:
                  {
                    if ($isOpeningMarker) {
                      $this->addToWordList ($this->subjectIndex);
                    }
                    break;
                  }
                  default:
                  {
                    if ($isOpeningMarker) {
                      $this->addToFallout ("Unknown word list marker \\$marker", false);
                    }
                    break;
                  }
                }
                // UserString1WordListEntryAddition:
                break;
              }
              default:
              {
                // This marker is not yet implemented. Add to fallout, plus any text that follows.
                $this->addToFallout ("Marker not yet implemented \\$marker, possible formatting error:", true);
                break;
              }
            }
          } else {
            // Here is an unknown marker. Add to fallout, plus any text that follows.
            $this->addToFallout ("Unknown marker \\$marker, formatting error:", true);
          }
        } else {
          // Here is no marker. Treat it as text.
          $this->odf_text_standard->addText ($currentItem);
          $this->odf_text_text_only->addText ($currentItem);
          $this->odf_text_text_and_note_citations->addText ($currentItem);
          $this->html_text_standard->addText ($currentItem); // Todo
          $this->html_text_linked->addText ($currentItem);
          $this->onlinebible_text->addText ($currentItem);
        }
      }
    }
  }


  /**
  * This function does the processing of the USFM code for one note,
  * formatting the document and extracting information.
  */
  private function processNote ()
  {
    for ( ; $this->chapterUsfmMarkersAndTextPointer < count ($this->chapterUsfmMarkersAndText); $this->chapterUsfmMarkersAndTextPointer++)
    {
      $currentItem = $this->chapterUsfmMarkersAndText[$this->chapterUsfmMarkersAndTextPointer];
      if (Filter_Usfm::isUsfmMarker ($currentItem))
      {
        // Store indicator whether the marker is an opening marker.
        $isOpeningMarker = Filter_Usfm::isOpeningMarker ($currentItem);
        // Clean up the marker, so we remain with the basic version, e.g. 'f'.
        $marker = Filter_Usfm::getMarker ($currentItem);
        if (array_key_exists ($marker, $this->styles)) 
        {
          $style = $this->styles[$marker];
          switch ($style['type']) 
          {
            case StyleTypeVerseNumber:
            {
              // Verse found. The note should have stopped here. Incorrect note markup.
              $this->addToFallout ("The note did not close at the end of the verse. The text is not correct.", false);
              break 2;
            }
            case StyleTypeFootEndNote:
            {
              switch ($style['subtype']) 
              {
                case FootEndNoteSubtypeFootnote:
                {
                  if ($isOpeningMarker) {
                    $this->ensureNoteParagraphStyle ($marker, $this->styles[$this->standardContentMarkerFootEndNote]);
                    $citation = $this->getNoteCitation ($style);
                    $this->odf_text_standard->addNote ($citation, $marker);
                    // Note citation in superscript in the document with text and note citations.
                    $currentTextStyle = $this->odf_text_text_and_note_citations->currentTextStyle;
                    $this->odf_text_text_and_note_citations->currentTextStyle = "superscript";
                    $this->odf_text_text_and_note_citations->addText ($citation);
                    $this->odf_text_text_and_note_citations->currentTextStyle = $currentTextStyle;
                    // Add space if the paragraph has text already.
                    if ($this->odf_text_notes->currentParagraphContent != "") {
                      $this->odf_text_notes->addText (" ");
                    }
                    // Add the note citation. And a no-break space after it.
                    $this->odf_text_notes->addText ($citation . Filter_Character::noBreakSpace());
                    // Open note in the web pages.
                    $this->html_text_standard->addNote ($citation, $this->standardContentMarkerFootEndNote); // Todo
                    $this->html_text_linked->addNote ($citation, $this->standardContentMarkerFootEndNote);
                    // Online Bible. Footnotes do not seem to behave as they ought in the Online Bible compiler.
                    // Just take them out, then.
                    //$this->onlinebible_text->addNote ();
                  } else {
                    break 3;
                  }
                  break;
                }
                case FootEndNoteSubtypeEndnote:
                {
                  if ($isOpeningMarker) {
                    $this->ensureNoteParagraphStyle ($marker, $this->styles[$this->standardContentMarkerFootEndNote]);
                    $citation = $this->getNoteCitation ($style);
                    $this->odf_text_standard->addNote ($citation, $marker, true);
                    // Note citation in superscript in the document with text and note citations.
                    $currentTextStyle = $this->odf_text_text_and_note_citations->currentTextStyle;
                    $this->odf_text_text_and_note_citations->currentTextStyle = "superscript";
                    $this->odf_text_text_and_note_citations->addText ($citation);
                    $this->odf_text_text_and_note_citations->currentTextStyle = $currentTextStyle;
                    // Open note in the web page.
                    $this->html_text_standard->addNote ($citation, $this->standardContentMarkerFootEndNote, true); // Todo
                    $this->html_text_linked->addNote ($citation, $this->standardContentMarkerFootEndNote, true);
                    // Online Bible.
                    //$this->onlinebible_text->addNote ();
                  } else {
                    break 3;
                  }
                  break;
                }
                case FootEndNoteSubtypeStandardContent:
                {
                  // The style of the standard content is already used in the note's body.
                  // If means that the text style should be cleared 
                  // in order to return to the correct style for the paragraph.
                  $this->odf_text_standard->closeTextStyle (true);
                  $this->odf_text_notes->closeTextStyle ();
                  $this->html_text_standard->closeTextStyle (true); // Todo
                  $this->html_text_linked->closeTextStyle (true);
                  break;
                }
                case FootEndNoteSubtypeContent:
                case FootEndNoteSubtypeContentWithEndmarker:
                {
                  if ($isOpeningMarker) {
                    $this->odf_text_standard->openTextStyle ($style, true);
                    $this->odf_text_notes->openTextStyle ($style);
                    $this->html_text_standard->openTextStyle ($style, true); // Todo
                    $this->html_text_linked->openTextStyle ($style, true);
                  } else {
                    $this->odf_text_standard->closeTextStyle (true);
                    $this->odf_text_notes->closeTextStyle ();
                    $this->html_text_standard->closeTextStyle (true); // Todo
                    $this->html_text_linked->closeTextStyle (true);
                  }
                  break;
                }
                case FootEndNoteSubtypeParagraph:
                {
                  // The style of this is not yet implemented.
                  $this->odf_text_standard->closeTextStyle (true);
                  $this->odf_text_notes->closeTextStyle ();
                  $this->html_text_standard->closeTextStyle (true); // Todo
                  $this->html_text_linked->closeTextStyle (true);
                  break;
                }
                default:
                {
                  break;
                }
              }
              // UserBool1NoteAppliesToApocrypha: For xref too?
              break;
            }
            case StyleTypeCrossreference:
            {
              switch ($style['subtype']) 
              {
                case CrossreferenceSubtypeCrossreference:
                {
                  if ($isOpeningMarker) {
                    $this->ensureNoteParagraphStyle ($marker, $this->styles[$this->standardContentMarkerCrossReference]);
                    $citation = $this->getNoteCitation ($style);
                    $this->odf_text_standard->addNote ($citation, $marker);
                    // Note citation in superscript in the document with text and note citations.
                    $currentTextStyle = $this->odf_text_text_and_note_citations->currentTextStyle;
                    $this->odf_text_text_and_note_citations->currentTextStyle = "superscript";
                    $this->odf_text_text_and_note_citations->addText ($citation);
                    $this->odf_text_text_and_note_citations->currentTextStyle = $currentTextStyle;
                    // Add a space if the paragraph has text already.
                    if ($this->odf_text_notes->currentParagraphContent != "") {
                      $this->odf_text_notes->addText (" ");
                    }
                    // Add the note citation. And a no-break space (NBSP) after it.
                    $this->odf_text_notes->addText ($citation . Filter_Character::noBreakSpace());
                    // Open note in the web page.
                    $this->ensureNoteParagraphStyle ($this->standardContentMarkerCrossReference, $this->styles[$this->standardContentMarkerCrossReference]);
                    $this->html_text_standard->addNote ($citation, $this->standardContentMarkerCrossReference); // Todo
                    $this->html_text_linked->addNote ($citation, $this->standardContentMarkerCrossReference);
                    // Online Bible.
                    //$this->onlinebible_text->addNote ();
                  } else {
                    break 3;
                  }
                  break;
                }
                case CrossreferenceSubtypeStandardContent:
                {
                  // The style of the standard content is already used in the note's body.
                  // If means that the text style should be cleared
                  // in order to return to the correct style for the paragraph.
                  $this->odf_text_standard->closeTextStyle (true);
                  $this->odf_text_notes->closeTextStyle ();
                  $this->html_text_standard->closeTextStyle (true); // Todo
                  $this->html_text_linked->closeTextStyle (true);
                  break;
                }
                case CrossreferenceSubtypeContent:
                case CrossreferenceSubtypeContentWithEndmarker:
                {
                  if ($isOpeningMarker) {
                    $this->odf_text_standard->openTextStyle ($style, true);
                    $this->odf_text_notes->openTextStyle ($style);
                    $this->html_text_standard->openTextStyle ($style, true); // Todo
                    $this->html_text_linked->openTextStyle ($style, true);
                  } else {
                    $this->odf_text_standard->closeTextStyle (true);
                    $this->odf_text_notes->closeTextStyle ();
                    $this->html_text_standard->closeTextStyle (true); // Todo
                    $this->html_text_linked->closeTextStyle (true);
                  }
                  break;
                }
                default:
                {
                  break;
                }
              }
              break;
            }
            default:
            {
              $this->addToFallout ("Marker not suitable in note context \\$marker", false);
              break;
            }
          }
        } else {
          // Here is an unknown marker. Add the marker to fallout, plus any text that follows.
          $this->addToFallout ("Unknown marker \\$marker", true);
        }
      } else {
        // Here is no marker. Treat it as text.
        $this->odf_text_standard->addNoteText ($currentItem);
        $this->odf_text_notes->addText ($currentItem);
        $this->html_text_standard->addNoteText ($currentItem); // Todo
        $this->html_text_linked->addNoteText ($currentItem);
        //$this->onlinebible_text->addText ($currentItem);
      }
    }
    
    // "Close" the current note, so that any following note text, if malformed, 
    // will be added to a new note, not to the last one created.
    $this->odf_text_standard->closeCurrentNote ();
    $this->odf_text_notes->closeTextStyle ();
    $this->html_text_standard->closeCurrentNote (); // Todo
    $this->html_text_linked->closeCurrentNote ();
    //$this->onlinebible_text->closeCurrentNote ();
  }


  /**
  * This creates and saves the Info Document.
  * The Info Document contains formatting information, collected from the USFM code.
  * $path: Path to the document.
  */
  public function produceInfoDocument ($path)
  {
    $database_books = Database_Books::getInstance ();

    $odf_text = new Odf_Text;
    
    // Number of chapters per book.
    $odf_text->newHeading1 (gettext ("Number of chapters per book"));
    foreach ($this->numberOfChaptersPerBook as $book => $chapterCount) {
      $line = $database_books->getEnglishFromId ($book) . " => " . $chapterCount;
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
    
    // Running headers.
    $odf_text->newHeading1 (gettext ("Running headers"));
    foreach ($this->runningHeaders as $item) {
      $line = $database_books->getEnglishFromId ($item['book']) . " (USFM " . $item['marker'] . ") => " . $item['value'];
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
    
    // Table of Contents entries.
    $odf_text->newHeading1 (gettext ("Long table of contents entries"));
    foreach ($this->longTOCs as $item) {
      $line = $database_books->getEnglishFromId ($item['book']) . " (USFM " . $item['marker'] . ") => " . $item['value'];
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
    $odf_text->newHeading1 (gettext ("Short table of contents entries"));
    foreach ($this->shortTOCs as $item) {
      $line = $database_books->getEnglishFromId ($item['book']) . " (USFM " . $item['marker'] . ") => " . $item['value'];
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }

    // Book abbreviations.
    $odf_text->newHeading1 (gettext ("Book abbreviations"));
    foreach ($this->bookAbbreviations as $item) {
      $line = $database_books->getEnglishFromId ($item['book']) . " (USFM " . $item['marker'] . ") => " . $item['value'];
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
        
    // Chapter specials.
    $odf_text->newHeading1 (gettext ("Publishing chapter labels"));
    foreach ($this->chapterLabels as $item) {
      $line = $database_books->getEnglishFromId ($item['book']) . " (USFM " . $item['marker'] . ") => " . $item['value'];
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
    $odf_text->newHeading1 (gettext ("Publishing alternate chapter numbers"));
    foreach ($this->publishedChapterMarkers as $item) {
      $line = $database_books->getEnglishFromId ($item['book']) . " (USFM " . $item['marker'] . ") => " . $item['value'];
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }

    // Word lists.
    $odf_text->newHeading1 (gettext ("Word list, glossary, dictionary entries"));
    foreach ($this->wordListGlossaryDictionary as $item) {
      $odf_text->newParagraph ();
      $odf_text->addText ($item);
    }
    $odf_text->newHeading1 (gettext ("Hebrew word list entries"));
    foreach ($this->hebrewWordList as $item) {
      $odf_text->newParagraph ();
      $odf_text->addText ($item);
    }
    $odf_text->newHeading1 (gettext ("Greek word list entries"));
    foreach ($this->greekWordList as $item) {
      $odf_text->newParagraph ();
      $odf_text->addText ($item);
    }
    $odf_text->newHeading1 (gettext ("Subject index entries"));
    foreach ($this->subjectIndex as $item) {
      $odf_text->newParagraph ();
      $odf_text->addText ($item);
    }
    
    // Other info.
    $odf_text->newHeading1 (gettext ("Other information"));
    foreach ($this->info as $line) {
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
    
    $odf_text->save ($path);
  }
  
  
  /**
  * This function produces the text of the current passage, e.g.: Genesis 1:1.
  * Returns: The passage text
  */
  private function getCurrentPassageText()
  {
    return Filter_Books::passageDisplay ($this->currentBookIdentifier, $this->currentChapterNumber, $this->currentVerseNumber);
  }
  

  /**
  * This function adds a string to the Info array, prefixed by the current passage.
  * $text: String to add to the Info array.
  * $next: If true, it also adds the text following the marker to the info, 
  * and removes this text from the USFM input stream.
  */
  private function addToInfo($text, $next = false)
  {
    $text = $this->getCurrentPassageText() . " " . $text;
    if ($next) {
      $text .= " " . Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
    }
    $this->info[] = $text;
  }



  /**
  * This function adds a string to the Fallout array, prefixed by the current passage.
  * $text: String to add to the Fallout array.
  * $next: If true, it also adds the text following the marker to the fallout, 
  * and removes this text from the USFM input stream.
  */
  private function addToFallout($text, $next = false)
  {
    $text = $this->getCurrentPassageText() . " " . $text;
    if ($next) {
      $text .= " " . Filter_Usfm::getTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
    }
    $this->fallout[] = $text;
  }



  /**
  * This function adds something to a word list array, prefixed by the current passage.
  * $list: which list to add the text to.
  * The word is extracted from the input USFM. The Usfm pointer points to the current marker,
  * and the text following that marker is added to the word list array.
  */
  private function addToWordList(&$list)
  {
    $text = Filter_Usfm::peekTextFollowingMarker ($this->chapterUsfmMarkersAndText, $this->chapterUsfmMarkersAndTextPointer);
    $text .= " (";
    $text .= $this->getCurrentPassageText();
    $text .= ")";
    $list[] = $text;
  }



  /**
  * This produces and saves the Fallout document.
  * $path: Path to the document.
  */
  public function produceFalloutDocument ($path)
  {
    $odf_text = new Odf_Text;
    $odf_text->newHeading1 (gettext ("Fallout"));
    foreach ($this->fallout as $line) {
      $odf_text->newParagraph ();
      $odf_text->addText ($line);
    }
    $odf_text->save ($path);
  }
  
  

  /**
  * This function produces ensures that a certain paragraph style is in the OpenDocument, 
  * and then opens a paragraph with this style
  * $style: The style to use.
  * $keepWithNext: Whether to keep this paragraph with the next one.
  */
  private function newParagraph ($style, $keepWithNext)
  {
    $marker = $style["marker"];
    if (!in_array ($marker, $this->createdOdfStyles)) {
      $fontsize = $style["fontsize"];
      $italic = $style["italic"];
      $bold = $style["bold"];
      $underline = $style["underline"];
      $smallcaps = $style["smallcaps"];
      $alignment = $style["justification"];
      $spacebefore = $style["spacebefore"];
      $spaceafter = $style["spaceafter"];
      $leftmargin = $style["leftmargin"];
      $rightmargin = $style["rightmargin"];
      $firstlineindent = $style["firstlineindent"];
      // Columns are not implemented at present. Reason:
      // I failed to copy and paste sections with columns between documents in LibreOffice.
      // So why implement something that does not work.
      // If it gets implemented, then sections are to be used in OpenDocument.
      $spancolumns = $style["spancolumns"];
      $dropcaps = 0;
      $this->odf_text_standard->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->odf_text_text_only->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->odf_text_text_and_note_citations->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->html_text_standard->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps); // Todo
      $this->html_text_linked->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->createdOdfStyles [] = $marker;
    }
    $this->odf_text_standard->newParagraph ($marker);
    $this->odf_text_text_only->newParagraph ($marker);
    $this->odf_text_text_and_note_citations->newParagraph ($marker);
    $this->html_text_standard->newParagraph ($marker); // Todo
    $this->html_text_linked->newParagraph ($marker);
  }
  
  
  
  /**
  * This applies the drop caps setting to the current paragraph style.
  * This is for the chapter number to appear in drop caps in the OpenDocument.
  * $dropCapsLength: Number of characters to put in drop caps.
  */
  private function applyDropCapsToCurrentParagraph ($dropCapsLength)
  {
    // To name a style according to the number of characters to put in drop caps,
    // e.g. a style name like p_c1 or p_c2 or p_c3.
    $combined_style = $this->odf_text_standard->currentParagraphStyle . "_" . $this->chapterMarker . $dropCapsLength;
    if (!in_array ($combined_style, $this->createdOdfStyles)) {
      $style = $this->styles[$this->odf_text_standard->currentParagraphStyle];
      $fontsize = $style["fontsize"];
      $italic = $style["italic"];
      $bold = $style["bold"];
      $underline = $style["underline"];
      $smallcaps = $style["smallcaps"];
      $alignment = $style["justification"];
      $spacebefore = $style["spacebefore"];
      $spaceafter = $style["spaceafter"];
      $leftmargin = $style["leftmargin"];
      $rightmargin = $style["rightmargin"];
      $firstlineindent = 0; // First line that contains the chapter number in drop caps is not indented.
      $spancolumns = $style["spancolumns"];
      $keepWithNext = false;
      $this->odf_text_standard->createParagraphStyle ($combined_style, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropCapsLength);
      $this->odf_text_text_only->createParagraphStyle ($combined_style, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropCapsLength);
      $this->odf_text_text_and_note_citations->createParagraphStyle ($combined_style, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropCapsLength);
      $this->createdOdfStyles [] = $combined_style;
    }
    $this->odf_text_standard->updateCurrentParagraphStyle ($combined_style);
    $this->odf_text_text_only->updateCurrentParagraphStyle ($combined_style);
    $this->odf_text_text_and_note_citations->updateCurrentParagraphStyle ($combined_style);
  }


  
  /**
  * This puts the chapter number in a frame in the current paragraph.
  * This is to put the chapter number in a frame so it looks like drop caps in the OpenDocument.
  * $chapterText: The text of the chapter indicator to put.
  */
  private function putChapterNumberInFrame ($chapterText)
  {
    $style = $this->styles[$this->chapterMarker];
    $this->odf_text_standard->placeTextInFrame ($chapterText, $this->chapterMarker, $style["fontsize"], $style["italic"], $style["bold"]);
    $this->odf_text_text_only->placeTextInFrame ($chapterText, $this->chapterMarker, $style["fontsize"], $style["italic"], $style["bold"]);
    $this->odf_text_text_and_note_citations->placeTextInFrame ($chapterText, $this->chapterMarker, $style["fontsize"], $style["italic"], $style["bold"]);
  }
  
  
  
  /**
  * This creates an entry in the $this->notecitations array.
  * $style: the style: an array of key => value.
  */
  private function createNoteCitation ($style)
  {
    // Create an entry in the notecitations array in this object, if it does not yet exist.
    if (!array_key_exists ($style['marker'], $this->notecitations)) {
      $numbering = $style['userint1'];
      $sequence = "1 2 3 4 5 6 7 8 9"; // Fallback sequence.
      if ($numbering == NoteNumbering123) $sequence = "";
      if ($numbering == NoteNumberingAbc) $sequence = "a b c d e f g h i j k l m n o p q r s t u v w x y z";
      if ($numbering == NoteNumberingUser) $sequence = $style['userstring1'];
      if ($sequence == "") $sequence = array ();
      else $sequence = explode (" ", $sequence);
      // Use of the above information:
      // The note will be numbered as follows:
      // If a $sequence is given, then this sequence is followed for the citations.
      // If no $sequence is given, then the note gets numerical citations.

      $restart = "chapter";
      $userint2 = $style['userint2'];
      if ($userint2 == NoteRestartNumberingNever) $restart = "never";
      if ($userint2 == NoteRestartNumberingEveryBook) $restart = "book";
      if ($userint2 == NoteRestartNumberingEveryChapter) $restart = "chapter";

      $this->notecitations[$style['marker']] = array ('sequence'  => $sequence, 'restart' => $restart, 'pointer' => 0);
    }
    
  }
  
  
  
  /**
  * This gets the note citation.
  * The first time that a xref is encountered, this function would return, e.g. 'a'.
  * The second time, it would return 'b'. Then 'c', 'd', 'e', and so on, up to 'z'.
  * Then it would restart with 'a'. And so on.
  * The note citation is the character that is put in superscript in the main body of Bible text.
  * $style: array with values for the note opening marker.
  * Returns: The character for the note citation.
  */
  private function getNoteCitation ($style)
  {
    // Get the raw note citation from the USFM. This could be, e.g. '+'.
    $nextText = $this->chapterUsfmMarkersAndText [$this->chapterUsfmMarkersAndTextPointer + 1];
    $citation = substr ($nextText, 0, 1);
    $nextText = ltrim (substr ($nextText, 1));
    $this->chapterUsfmMarkersAndText [$this->chapterUsfmMarkersAndTextPointer + 1] = $nextText;
    $citation = trim ($citation);
    if ($citation == "+") {
      $marker = $style['marker'];
      $sequence = $this->notecitations[$marker]['sequence'];
      $pointer = $this->notecitations[$marker]['pointer'];
      if (count ($sequence) == 0) {
        $citation = ++$pointer;
      } else {
        $citation = $sequence [$pointer];
        $pointer++;
        if ($pointer >= count ($sequence)) $pointer = 0;
      }
      $this->notecitations[$marker]['pointer'] = $pointer;
    } else if ($citation == "-") {
      $citation = "";
    }
    return $citation;    
  }


  /**
  * This resets selected note citation data.
  * Resetting means that the note citations start to count afresh.
  * $moment: what type of reset to apply, e.g. 'chapter' or 'book'.
  */
  private function resetNoteCitations ($moment)
  {
    foreach ($this->notecitations as &$notecitation) {
      if ($notecitation['restart'] == $moment) {
        $notecitation['pointer'] = 0;
      }
    }
  }



  /**
  * This function ensures that a certain paragraph style for a note is present in the OpenDocument.
  * $marker: Which note, e.g. 'f' or 'x' or 'fe'.
  * $style: The style to use.
  */
  private function ensureNoteParagraphStyle ($marker, $style)
  {
    if (!in_array ($marker, $this->createdOdfStyles)) {
      $fontsize = $style["fontsize"];
      $italic = $style["italic"];
      $bold = $style["bold"];
      $underline = $style["underline"];
      $smallcaps = $style["smallcaps"];
      $alignment = $style["justification"];
      $spacebefore = $style["spacebefore"];
      $spaceafter = $style["spaceafter"];
      $leftmargin = $style["leftmargin"];
      $rightmargin = $style["rightmargin"];
      $firstlineindent = $style["firstlineindent"];
      $spancolumns = false;
      $keepWithNext = false;
      $dropcaps = 0;
      $this->odf_text_standard->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->odf_text_text_only->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->odf_text_text_and_note_citations->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->odf_text_notes->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, 0, 0, 0, 0, 0, $keepWithNext, $dropcaps);
      $this->html_text_standard->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps); // Todo
      $this->html_text_linked->createParagraphStyle ($marker, $fontsize, $italic, $bold, $underline, $smallcaps, $alignment, $spacebefore, $spaceafter, $leftmargin, $rightmargin, $firstlineindent, $keepWithNext, $dropcaps);
      $this->createdOdfStyles [] = $marker;
    }
  }
  
  
  
}

?>


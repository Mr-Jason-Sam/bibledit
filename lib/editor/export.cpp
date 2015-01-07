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


#include <editor/export.h>
#include <filter/string.h>
#include <filter/url.h>
#include <filter/usfm.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <styles/logic.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <database/logs.h>


Editor_Export::Editor_Export (void * webserver_request_in)
{
  webserver_request = webserver_request_in;
}


Editor_Export::~Editor_Export ()
{
  //xmlDocDump (stdout, document);
  xmlFreeDoc (document);
  htmlFreeParserCtxt (context);
}


void Editor_Export::load (string html)
{
  // The online editor may insert non-breaking spaces. Convert them to normal ones.
  html = filter_string_str_replace ("&nbsp;", " ", html);
  
  // The user may add several spaces in sequence. Convert them to single spaces.
  html = filter_string_str_replace ("   ", " ", html);
  html = filter_string_str_replace ("  ", " ", html);
  
  // DOMDocument deals well with imperfect markup, but it may throw warnings to the default error handler.
  // Therefore keep the errors separate.
  xmlGenericErrorFunc handler = (xmlGenericErrorFunc) error_handler;
  initGenericErrorDefaultFunc (&handler);
  
  htmlParserCtxtPtr context = htmlNewParserCtxt();

  // To help loadHTML() process utf8 correctly, set the correct meta tag before any other text.
  string prefix =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head>\n"
  "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n"
  "</head>\n"
  "<body>\n";
  string suffix =
  "\n"
  "</body>\n"
  "</html>\n";
  string xml = prefix + html + suffix;
  document = htmlCtxtReadMemory (context, xml.c_str(), xml.length(), "", "UTF-8", HTML_PARSE_RECOVER);
}


void Editor_Export::stylesheet (string stylesheet)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  styles.clear ();
  noteOpeners.clear ();
  characterStyles.clear ();
  vector <string> markers = request->database_styles()->getMarkers (stylesheet);
  // Load the style information into the object.
  for (string & marker : markers) {
    Database_Styles_Item style = request->database_styles()->getMarkerData (stylesheet, marker);
    styles [marker] = style;
    // Get markers with should not have endmarkers.
    bool suppress = false;
    int type = style.type;
    int subtype = style.subtype;
    if (type == StyleTypeVerseNumber) suppress = true;
    if (type == StyleTypeFootEndNote) {
      suppress = true;
      if (subtype == FootEndNoteSubtypeFootnote) noteOpeners.insert (marker);
      if (subtype == FootEndNoteSubtypeEndnote) noteOpeners.insert (marker);
      if (subtype == FootEndNoteSubtypeContentWithEndmarker) suppress = false;
      if (subtype == FootEndNoteSubtypeParagraph) suppress = false;
    }
    if (type == StyleTypeCrossreference) {
      suppress = true;
      if (subtype == CrossreferenceSubtypeCrossreference) noteOpeners.insert (marker);
      if (subtype == CrossreferenceSubtypeContentWithEndmarker) suppress = false;
    }
    if (type == StyleTypeTableElement) suppress = true;
    if (suppress) suppressEndMarkers.insert (marker);
  }
}


void Editor_Export::run ()
{
  preprocess ();
  process ();
  postprocess ();
}


void Editor_Export::process ()
{
  // Walk the tree to retrieve the "p" elements, then process them.
  xmlNodePtr mainnode = xmlDocGetRootElement (document);
  mainnode = mainnode->xmlChildrenNode;
  while (mainnode != NULL) {
    if ((!xmlStrcmp (mainnode->name, (const xmlChar *)"body"))) {
      xmlNodePtr pnode = mainnode->xmlChildrenNode;
      while (pnode != NULL) {
        if ((!xmlStrcmp (pnode->name, (const xmlChar *)"p"))) {
          processNode (pnode);
        }
        pnode = pnode->next;
      }
    }
    mainnode = mainnode->next;
  }
}


string Editor_Export::get ()
{
  // Generate the USFM as one string.
  string usfm = filter_string_implode (output, "\n");
  
  usfm = cleanUSFM (usfm);
  
  return usfm;
}


void Editor_Export::processNode (xmlNodePtr node)
{
  switch (node->type) {
    case XML_ELEMENT_NODE:
    {
      openElementNode (node);
      processNodeChildren (node);
      closeElementNode (node);
      break;
    }
    case XML_ATTRIBUTE_NODE:
    {
      processAttributeNode (node);
      break;
    }
    case XML_TEXT_NODE:
    {
      processTextNode (node);
      break;
    }
    case XML_CDATA_SECTION_NODE:
    case XML_ENTITY_REF_NODE:
    case XML_ENTITY_NODE:
    case XML_PI_NODE:
    case XML_COMMENT_NODE:
    case XML_DOCUMENT_NODE:
    case XML_DOCUMENT_TYPE_NODE:
    case XML_DOCUMENT_FRAG_NODE:
    case XML_NOTATION_NODE:
    {
      break;
    }
    default:
    {
      string nodename ((char *) node->name);
      Database_Logs::log ("Unknown DOM node " + nodename + " while saving editor text");
      break;
    }
  }
}


void Editor_Export::openElementNode (xmlNodePtr node)
{
  // The tag and class names of this element node.
  string tagName ((char *) node->name);
  string className;
  xmlChar * property = xmlGetProp (node, BAD_CAST "class");
  if (property) {
    className = (char *) property;
    xmlFree (property);
  }
  
  if (tagName == "p")
  {
    // While editing, it may occur that the p element does not have a class.
    // Use the 'p' class in such cases.
    if (className.empty ()) className = "p";
    if (className == "mono") {
      // Class 'mono': The editor has the full USFM in the text.
      mono = true;
    } else {
      // Start the USFM line with a marker with the class name.
      currentLine += usfm_get_opening_usfm (className);
    }
  }
  
  if (tagName == "span")
  {
    if (className == "v")  {
      // Handle the verse.
      flushLine ();
      openInline (className);
    }
    else if (className.empty ()) {
      // Normal text is wrapped in elements without a class attribute.
    } else {
      // Handle remaining class attributes for inline text.
      openInline (className);
    }
  }
  
  if (tagName == "a")
  {
    processNoteCitation (node);
  }
  
}


void Editor_Export::processNodeChildren (xmlNodePtr node)
{
  xmlNodePtr childNode = node->xmlChildrenNode;
  while (childNode != NULL) {
    processNode (childNode);
    childNode = childNode->next;
  }
}


void Editor_Export::closeElementNode (xmlNodePtr node)
{
  // The tag and class names of this element node.
  string tagName ((char *) node->name);
  string className;
  xmlChar * property = xmlGetProp (node, BAD_CAST "class");
  if (property) {
    className = (char *) property;
    xmlFree (property);
  }
  
  if (tagName == "p")
  {
    // While editing it happens that the p element does not have a class.
    // Use the 'p' class in such cases.
    if (className == "") className = "p";
    
    if (noteOpeners.find (className) != noteOpeners.end()) {
      // Deal with note closers.
      currentLine += usfm_get_closing_usfm (className);
    } else {
      // Normally a p element closes the USFM line.
      flushLine ();
      mono = false;
      // Clear active character styles.
      characterStyles.clear();
    }
  }
  
  if (tagName == "span")
  {
    // Do nothing for monospace elements, because the USFM would be the text nodes only.
    if (mono) return;
    // Do nothing without a class.
    if (className.empty()) return;
    // Do nothing if no endmarkers are supposed to be produced.
    if (suppressEndMarkers.find (className) == suppressEndMarkers.end()) return;
    // Add closing USFM, optionally closing embedded tags in reverse order.
    vector <string> classes = filter_string_explode (className, ' ');
    characterStyles = filter_string_array_diff (characterStyles, classes);
    reverse (classes.begin(), classes.end());
    for (unsigned int offset = 0; offset < classes.size(); offset++) {
      bool embedded = (classes.size () > 1) && (offset == 0);
      if (!characterStyles.empty ()) embedded = true;
      currentLine += usfm_get_closing_usfm (classes [offset], embedded);
    }
  }
  
  if (tagName == "a")
  {
    // Do nothing for note citations in the text.
  }
  
}


void Editor_Export::processAttributeNode (xmlNodePtr node)
{
  string tagName ((char *) node->name);
  Database_Logs::log ("Unprocessed XML_ATTRIBUTE_NODE while saving editor text " + tagName);
}


void Editor_Export::processTextNode (xmlNodePtr node)
{
  // Add the text to the current USFM line.
  xmlChar * contents = xmlNodeListGetString (document, node, 1);
  if (contents) {
    string text ((char *) contents);
    currentLine += text;
    xmlFree (contents);
  }
}


void Editor_Export::openInline (string className)
{
  // It has been observed that the <span> elements of the character styles may be embedded, like so:
  // The <span class="add">
  //   <span class="nd">Lord God</span>
  // is calling</span> you</span><span>.</span>
  vector <string> classes = filter_string_explode (className, ' ');
  for (unsigned int offset = 0; offset < classes.size(); offset++) {
    bool embedded = (characterStyles.size () + offset) > 0;
    currentLine += usfm_get_opening_usfm (classes[offset], embedded);
  }
  // Store active character styles in some cases.
  bool store = true;
  if (suppressEndMarkers.find (className) != suppressEndMarkers.end ()) store = false;
  if (processingNote) store = false;
  if (store) {
    characterStyles.insert (characterStyles.end(), classes.begin(), classes.end());
  }
}


void Editor_Export::processNoteCitation (xmlNodePtr node)
{
  if (node) {};
  /* Todo
  // The note citation in the text will have the "1" or the "2", and so on, till "9". Remove it.
  if ($node->hasChildNodes ()) {
    for ($node->childNodes as $childnode) {
      $node->removeChild ($childnode);
    }
  }
  
  // Get more information about the footnote to retrieve.
  $href = $node->getAttribute ("href");
  $id = substr ($href, 1);
  
  // Sample footnote body.
  // <p class="x"><a href="#citation1" id="note1">x</a><span> </span><span>+ 2 Joh. 1.1</span></p>
  
  // XPath to retrieve the note contents.
  $xpath = new DOMXPath ($this->document);
  $path = "//a[@id='$id']";
  $nodeList = $xpath->query ($path);
  
  // There should be only one relevant note node.
  if ($nodeList->length != 1) {
    $database_logs = Database_Logs::getInstance ();
    Database_Logs::log ("Discarding note with id $id and href $href");
    return;
  }
  
  // Get the 'a' element, its 'p' parent, and then remove that 'a' element.
  // So we remain with:
  // <p class="x"><span> </span><span>+ 2 Joh. 1.1</span></p>
  $aElement = $nodeList->item (0);
  $pElement = $aElement->parentNode;
  $pElement->removeChild ($aElement);
  
  // Preserve active character styles in the main text, and reset them for the note.
  $characterStyles = $this->characterStyles;
  $this->characterStyles = array ();
  
  // Process this 'p' element.
  $this->processingNote = true;
  $this->processNode ($pElement);
  $this->processingNote = false;
  
  // Restore the active character styles for the main text.
  $this->characterStyles = $characterStyles;
  
  // Remove this element so it can't be processed again.
  $parentNode = $pElement->parentNode;
  $parentNode->removeChild ($pElement);
  */
}


string Editor_Export::cleanUSFM (string usfm)
{
  // The user may accidentally omit or erase the note caller.
  // The note caller is one character that immediately follows the note opener.
  // E.g.: \f + ...\f*.
  // Check for missing note caller, and if it's not there, add the default "+".
  // Also replace a double space after a note opener.
  for (string noteOpener : noteOpeners) {
    string opener = usfm_get_opening_usfm (noteOpener);
    usfm = filter_string_str_replace (opener + " ", opener, usfm);
    size_t pos = unicode_string_strpos (usfm, opener);
    while (pos != string::npos) {
      bool isClean = true;

      // Check that the character that follows the note opener is a non-space.
      size_t pos2 = pos + unicode_string_length (opener);
      string character = unicode_string_substr (usfm, pos2, 1);
      if (character == " ") isClean = false;
      
      // Check that the following character is not a space.
      pos2++;
      character = unicode_string_substr (usfm, pos2, 1);
      if (character != " ") isClean = false;
      
      // Fix the note caller if necessary.
      if (!isClean) {
        Database_Logs::log ("Fixing note caller in " + usfm);
        pos2--;
        usfm = unicode_string_substr (usfm, 0, pos2) + "+" + unicode_string_substr (usfm, pos2);
      }
      
      // Next iteration.
      pos = unicode_string_strpos (usfm, opener, pos + 5);
    }
  }
  
  return usfm;
}


void Editor_Export::preprocess ()
{
  output.clear ();
  currentLine.clear ();
  mono = false;
  //document->encoding = BAD_CAST "UTF-8";
  //document->preserveWhiteSpace = false;
}


// Log errors to the logbook.
void Editor_Export::error_handler (void *ctx, const char *msg, ...)
{
  if (ctx) {};
  char buf [256];
  va_list arg_ptr;
  va_start (arg_ptr, msg);
  vsnprintf (buf, 256, msg, arg_ptr);
  va_end (arg_ptr);
  string error (gettext ("Saving Editor text") + ": ");
  error.append (buf);
  Database_Logs::log (error);
  return;
}


void Editor_Export::flushLine ()
{
  if (!currentLine.empty ()) {
    // Trim so that '\p ' becomes '\p', for example.
    currentLine = filter_string_trim (currentLine);
    output.push_back (currentLine);
    currentLine.clear ();
  }
}


void Editor_Export::postprocess ()
{
  // Flush any last USFM line being built.
  flushLine ();
}



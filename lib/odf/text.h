/*
Copyright (©) 2003-2014 Teus Benschop.

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


#ifndef INCLUDED_ODF_TEXT_H
#define INCLUDED_ODF_TEXT_H


#include <config/libraries.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <database/styles.h>


class Odf_Text
{
public:
  Odf_Text (string bible_in);
  ~Odf_Text ();
  void createPageBreakStyle ();
  void save (string name);
  string currentParagraphStyle;
  string currentParagraphContent;
  vector <string> currentTextStyle;
private:
  string bible;
  string unpackedOdtFolder;
  xmlDocPtr contentDom; // The content.xml DOMDocument.
  xmlNodePtr officeTextDomNode; // The office:text DOMNode.
  xmlDocPtr stylesDom; // The styles.xml DOMDocument.
  vector <string> createdStyles; // An array with styles already created in the $stylesDom.
  xmlNodePtr officeStylesDomNode; // The office:styles DOMNode.
  xmlNodePtr officeAutomaticStylesDomNode; // The office:automatic-styles DOMNode.
  xmlNodePtr currentTextPDomElement; // The current text:p DOMElement.
  string currentTextPDomElementNameNode; // The DOMAttr of the name of the style of the current text:p element.
  int frameCount;
  int noteCount;
  xmlNodePtr noteTextPDomElement; // The text:p DOMElement of the current footnote, if any.
  vector <string> currentNoteTextStyle;
  void initialize_content_xml ();
  void initialize_styles_xml ();
};


#endif





  

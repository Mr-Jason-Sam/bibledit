/*
 ** Copyright (©) 2003-2009 Teus Benschop.
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
#include <glib.h>
#include "windoweditor.h"
#include "help.h"
#include "windows.h"
#include "keyterms.h"
#include "tiny_utilities.h"

WindowEditor::WindowEditor(const ustring & project_name, GtkAccelGroup * accelerator_group, bool startup):WindowBase(widEditor, project_name, startup, 0)
// Text editor.
{
  // Signalling buttons.
  new_verse_signal = gtk_button_new();
  new_styles_signal = gtk_button_new();
  quick_references_button = gtk_button_new();
  word_double_clicked_signal = gtk_button_new();
  reload_signal = gtk_button_new();
  changed_signal = gtk_button_new();

  // Gui.  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  // Create the new editor.
  editor = new Editor(vbox, project_name);
  g_signal_connect((gpointer) editor->textview, "visibility-notify-event", G_CALLBACK(on_visibility_notify_event), gpointer(this));
  g_signal_connect((gpointer) editor->new_verse_signal, "clicked", G_CALLBACK(on_new_verse_signalled), gpointer(this));
  g_signal_connect((gpointer) editor->new_styles_signal, "clicked", G_CALLBACK(on_new_styles_signalled), gpointer(this));
  g_signal_connect((gpointer) editor->quick_references_button, "clicked", G_CALLBACK(on_quick_references_signalled), gpointer(this));
  g_signal_connect((gpointer) editor->word_double_clicked_signal, "clicked", G_CALLBACK(on_word_double_click_signalled), gpointer(this));
  g_signal_connect((gpointer) editor->reload_signal, "clicked", G_CALLBACK(on_reload_signalled), gpointer(this));
  g_signal_connect((gpointer) editor->changed_signal, "clicked", G_CALLBACK(on_changed_signalled), gpointer(this));
}

WindowEditor::~WindowEditor()
{
  gtk_widget_destroy (new_verse_signal);
  gtk_widget_destroy (new_styles_signal);
  gtk_widget_destroy (quick_references_button);
  gtk_widget_destroy (word_double_clicked_signal);
  gtk_widget_destroy (reload_signal);
  gtk_widget_destroy (changed_signal);
  delete editor;
}

void WindowEditor::go_to(const Reference & reference)
// Let the editor go to a reference.
{
  // Find out what needs to be changed: book, chapter and/or verse.
  bool new_book = (reference.book != editor->current_reference.book);
  editor->current_reference.book = reference.book;
  bool new_chapter = (reference.chapter != editor->current_reference.chapter);
  editor->current_reference.chapter = reference.chapter;
  bool new_verse = (reference.verse != editor->current_reference.verse);
  editor->current_reference.verse = reference.verse;

  // Save the editor if need be.
  if (new_book || new_chapter) {
    editor->chapter_save();
  }
  // With a new book, also load a new chapter.
  if (new_book) {
    new_chapter = true;
    editor->book_set(reference.book);
  }
  // Deal with a new chapter.
  if (new_chapter) {
    // Load chapter in Editor, if need be.
    editor->chapter_load(reference.chapter);
    // When loading a new chapter, there is also a new verse.
    new_verse = true;
  }
  // New reference handling.  
  if (new_book || new_chapter || new_verse) {
    // Position the cursor properly.
    // The positioning will be done whenever Gtk is idle.
    // This is because sometimes Gtk is slow in loading a new chapter.
    // So if the cursor positioning is done straight after loading,
    // it will not work, as there is no text loaded yet.
    // But here we deal with that so that a delay is no longer needed. The trick:
    // Handle all pending events in GTK.
    while (gtk_events_pending())
      gtk_main_iteration();
    editor->position_cursor_at_verse(reference.verse, false);
  }
  // Highlighting of searchwords.
  if (editor->go_to_new_reference_highlight) {
    editor->highlight_searchwords();
    editor->go_to_new_reference_highlight = false;
  }
}


void WindowEditor::load_dictionaries()
{
  editor->load_dictionaries();
}


void WindowEditor::undo()
{
  editor->undo();
}


void WindowEditor::redo()
{
  editor->redo();
}


bool WindowEditor::can_undo()
{
  return editor->can_undo();
}


bool WindowEditor::can_redo()
{
  return editor->can_redo();
}


EditorTextViewType WindowEditor::last_focused_type()
{
  return editor->last_focused_type();
}


vector <Reference> WindowEditor::quick_references()
{
  return editor->quick_references;
}


Reference WindowEditor::current_reference()
{
  return editor->current_reference;
}


ustring WindowEditor::current_verse_number()
{
  return editor->current_verse_number;
}


ustring WindowEditor::project()
{
  return editor->project;
}


ustring WindowEditor::text_get_selection()
{
  return editor->text_get_selection();
}


void WindowEditor::text_erase_selection()
{
  return editor->text_erase_selection();
}


GtkTextBuffer * WindowEditor::last_focused_textbuffer()
{
  return editor->last_focused_textbuffer();
}


void WindowEditor::text_insert(ustring text)
{
  editor->text_insert(text);
}


void WindowEditor::go_to_new_reference_highlight_set() 
{
  editor->go_to_new_reference_highlight = true;
}


ustring WindowEditor::word_double_clicked_text()
{
  return editor->word_double_clicked_text;
}


bool WindowEditor::editable()
{
  return editor->editable;
}


void WindowEditor::insert_note(const ustring& marker, const ustring& rawtext, GtkTextIter * iter, bool render)
{
  editor->insert_note (marker, rawtext, iter, render);
}


ustring WindowEditor::get_chapter()
{
  return editor->get_chapter();
}


void WindowEditor::insert_table(const ustring& rawtext, GtkTextIter * iter)
{
  editor->insert_table (rawtext, iter);
}


void WindowEditor::chapter_load(unsigned int chapter_in, vector <ustring> * lines_in)
{
  editor->chapter_load (chapter_in, lines_in);
}


void WindowEditor::chapter_save()
{
  editor->chapter_save();
}


unsigned int WindowEditor::reload_chapter_number()
{
  return editor->reload_chapter_number;
}


void WindowEditor::apply_style(const ustring& marker)
{
  editor->apply_style (marker);
}


set <ustring> WindowEditor::get_styles_at_cursor()
{
  return editor->get_styles_at_cursor();
}

void WindowEditor::create_or_update_formatting_data()
{
  editor->create_or_update_formatting_data();
}


void WindowEditor::set_font()
{
  editor->set_font();
}


Editor * WindowEditor::editor_get()
{
  return editor;
}


unsigned int WindowEditor::book()
{
  return editor->book;
}


unsigned int WindowEditor::chapter()
{
  return editor->chapter;
}


void WindowEditor::on_new_verse_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_new_verse();
}


void WindowEditor::on_new_verse()
{
  gtk_button_clicked (GTK_BUTTON (new_verse_signal));
}


void WindowEditor::on_new_styles_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_new_styles();
}


void WindowEditor::on_new_styles()
{
  gtk_button_clicked (GTK_BUTTON (new_styles_signal));
}


void WindowEditor::on_quick_references_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_quick_references();
}


void WindowEditor::on_quick_references()
{
  gtk_button_clicked (GTK_BUTTON (quick_references_button));
}


void WindowEditor::on_word_double_click_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_word_double_click();
}


void WindowEditor::on_word_double_click()
{
  gtk_button_clicked (GTK_BUTTON (word_double_clicked_signal));
}


void WindowEditor::on_reload_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_reload();
}


void WindowEditor::on_reload()
{
  gtk_button_clicked (GTK_BUTTON (reload_signal));
}


void WindowEditor::on_changed_signalled(GtkButton *button, gpointer user_data)
{
  ((WindowEditor *) user_data)->on_changed();
}


void WindowEditor::on_changed()
{
  gtk_button_clicked (GTK_BUTTON (changed_signal));
}



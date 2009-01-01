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

#include "keyboard.h"
#include <gdk/gdkkeysyms.h>

bool keyboard_enter_pressed(GdkEventKey * event)
// Returns true if the event was "Enter", i.e. if Enter was pressed on the keyboard.
{
  switch (event->keyval) {
  case GDK_Return:
  case GDK_KP_Enter:
    return true;
  }
  return false;
}

bool keyboard_delete_pressed(GdkEventKey * event)
// Returns true if the event was "Delete", i.e. if Delete was pressed on the keyboard.
{
  switch (event->keyval) {
  case GDK_Delete:
  case GDK_KP_Delete:
    return true;
  }
  return false;
}

bool keyboard_insert_pressed(GdkEventKey * event)
// Returns true if the event was "Insert", i.e. if Insert was pressed on the keyboard.
{
  switch (event->keyval) {
  case GDK_Insert:
  case GDK_KP_Insert:
    return true;
  }
  return false;
}

bool keyboard_up_arrow_pressed(GdkEventKey * event)
// Returns true if the up arrow was pressed.
{
  switch (event->keyval) {
  case GDK_Up:
  case GDK_KP_Up:
    return true;
  }
  return false;
}

bool keyboard_left_arrow_pressed(GdkEventKey * event)
// Returns true if the left arrow was pressed.
{
  switch (event->keyval) {
  case GDK_Left:
  case GDK_KP_Left:
    return true;
  }
  return false;
}

bool keyboard_right_arrow_pressed(GdkEventKey * event)
// Returns true if the right arrow was pressed.
{
  switch (event->keyval) {
  case GDK_Right:
  case GDK_KP_Right:
    return true;
  }
  return false;
}

bool keyboard_down_arrow_pressed(GdkEventKey * event)
// Returns true if the down arrow was pressed.
{
  switch (event->keyval) {
  case GDK_Down:
  case GDK_KP_Down:
    return true;
  }
  return false;
}

bool keyboard_page_up_pressed(GdkEventKey * event)
// Returns true if the page up key was pressed.
{
  switch (event->keyval) {
  case GDK_Page_Up:
  case GDK_KP_Page_Up:
    return true;
  }
  return false;
}

bool keyboard_control_state(GdkEventButton * event)
// Returns true if the Ctrl key was down at the mouse click.
{
  guint modifiers;
  modifiers = gtk_accelerator_get_default_mod_mask();
  if ((event->state & modifiers) == GDK_CONTROL_MASK)
    return true;
  return false;
}

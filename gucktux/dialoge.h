/***************************************************************************
                          dialoge.h  -  description
                             -------------------
    begin                : Mit Sep 25 2002
    copyright            : (C) 2002 by Abadon
    email                : 3999@freenet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef DIALOGE_H
#define DIALOGE_H

void but_about();
void but_debug(gpointer callback_data, guint callback_action, GtkWidget *widget);
void but_telnet_dialog(gpointer button, GtkWidget *widget);
void insertText(GtkWidget *view, GtkTextBuffer *buffer, gchar *s, gboolean scroll);

#endif

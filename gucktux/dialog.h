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
#ifndef DIALOG_H
#define DIALOG_H
#include <gtk/gtk.h>

void but_ftpLog();
void but_help();
void but_about();
gboolean showChannelData(GtkTreeView *treeview, GdkEventButton *event, gpointer data);
void but_keyLayout();
void but_debug(gpointer callback_data, guint callback_action, GtkWidget *widget);
void but_serial_dialog(gpointer button, GtkWidget *widget);
void but_telnet_dialog(gpointer button, GtkWidget *widget);
void insertText(GtkWidget *view, gchar *s, gboolean scroll);
#endif

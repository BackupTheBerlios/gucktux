/***************************************************************************
                          tools.h  -  description
                             -------------------
    begin                : Sam Sep 21 2002
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
#ifndef TOOLS_H
#define TOOLS_H

#include "guis.h"
#include "gsatedit.h"
#define CFG_ENTRY_MAXLEN 40

struct s_WDesktopInfo {
     gint width;
     gint height;
     gint depth;
};
typedef struct s_WDesktopInfo WDesktopInfo;

WDesktopInfo* w_get_desktop_info();
gint  cmpStr(unsigned char *txt1, unsigned char *txt2);
gint  getServicePic(gint serviceType);
gint  strFind(gchar *text1, gchar *text2);
gchar *get_path (gchar *filename);
gchar *cfg_write(gchar *section, gchar *key, gchar *value);
gchar *cfg_read (gchar *section, gchar *key, gchar *value);
gchar *txtOut(gchar *text);
gchar *txtIn (gchar *text);
void clearUndo();
void clearRedo();
void but_Undo(gpointer callback_data, guint  callback_action, GtkWidget *widget);
void but_Redo(gpointer callback_data, guint  callback_action, GtkWidget *widget);
void pushUndo(gint view, gint action, gint pos, GNode *node, gboolean callFromRedo);
void pushRedo(gint view, gint action, gint pos, GNode *node, gboolean callFromUndo);
void but_writeConfig();
void but_saveAktAsRTF();
void center_window(GtkWindow *);
void but_saveAktBouquet(gpointer callback_data, guint  callback_action, GtkWidget *widget);
void but_saveAllBouquet(gpointer callback_data, guint  callback_action, GtkWidget *widget);

#endif

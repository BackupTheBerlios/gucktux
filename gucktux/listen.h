/***************************************************************************
                          listen.h  -  description
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

 
#ifndef LISTEN_H
#define LISTEN_H


void countChannels();

gboolean key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean entry_keypressed(GtkEntry *searchEntry);
void entry_focus(GtkEntry *searchEntry);
void entry_doSearch(GtkEntry *searchEntry);
void but_copyAll      (gpointer callback_data, GtkWidget *widget);
void but_move         (gpointer callback_data, gpointer direction);
void but_showItems    (gpointer callback_data, GtkWidget *widget);
void but_showCount    (gpointer callback_data, GtkWidget *widget);
void but_showHeader   (gpointer callback_data, GtkWidget *widget);
void but_del_entries  (gpointer callback_data, GtkWidget *widget);
void but_showIconText (gpointer callback_data, gpointer   button);
void but_addBouquet(gpointer button, GtkWidget *widget);
void but_copy_channels(gpointer callback_data, gpointer action);
void but_toggleViewMode(GtkWidget *view, gboolean fromUndo);
void fill_left_listview();
void fill_channel_listview(GNode *right_bouquet);
void fill_bouquet_listview();
void clear_node(GNode *root_node);
void clear_left_listview();
void clear_right_listview();
void tree_drag_data_received(GtkWidget *widget,GdkDragContext *dc,gint x,gint y,
		GtkSelectionData *selection,guint info,guint t,gpointer data);
void tree_drag_data_get(GtkWidget *widget,GdkDragContext *dc,
		GtkSelectionData *selectionData,guint info,guint t,gpointer data);
void updateStatusbar(statusbar *status);
void swap_row(gint view, gint pos1, gint pos2);
void move_row(gint view, gint pos1, gint pos2);
void prepareZap();
void zapToChannel();
GNode *remove_node(GNode *node_channel);
GNode *addBouquet(gchar *bouquetName);
gint treeview_sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,gpointer column);
gint cell_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer data);
gint column_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data);
gint bouquet_selected(GtkTreeView *treeview, GtkTreePath *arg1,
			GtkTreeViewColumn *arg2, gpointer user_data);
gboolean bouquet_dblClick(GtkTreeView *treeview, GdkEventButton *event, gpointer user_data);
gint dblClick(GtkTreeView *treeview, GdkEventButton *event, gpointer user_data);

#endif

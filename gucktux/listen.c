/***************************************************************************
                          listen.c  -  description
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>
#include "gsatedit.h"
#include "ftp.h"
#include "tools.h"
#include "telnet.h"
#include "dialog.h"
#include "listen.h"

gint zapActive =0;
gint preActive =0;
////////////////////////////////////////////////////////////////////////////////
// Count channels in actual Bouquets (TV, Radio, Data separatly).
////////////////////////////////////////////////////////////////////////////////
void countChannels(){
	gchar strNum[8];
	GtkTreeIter iter;
	GNode *node_channel;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
	gint sumTV = 0, sumRadio = 0, sumData =0;

	if (!g_node_first_child(MW_GET("AKT_BOUQUET"))) return;
	gtk_tree_model_get_iter_first(model, &iter);
	while(1){
		gtk_tree_model_get(model, &iter, RIGHT_CHANNEL_LIST__NODE, &node_channel, -1);
		switch(((channelEntry*)node_channel->data)->serviceType){
			case RADIO:
				sprintf(strNum, "%.3d", ++sumRadio);
				break;
			case DATA:
				sprintf(strNum, "%.3d", ++sumData);
				break;
			default:
				sprintf(strNum, "%.3d", ++sumTV);
				break;
		}
		gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter, RIGHT_CHANNEL_LIST__NUMBER, strNum,-1);
		if (gtk_tree_model_iter_next(model, &iter) == FALSE) break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Search for an entry in left listview.
////////////////////////////////////////////////////////////////////////////////
void entry_doSearch(GtkEntry *searchEntry){
	static gint old_pos = -1;
	gint pos = 0;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	gchar *entry_text;
	GNode *node_channel;
	gchar path_string[8];
	GtkWidget *treeview= MW_GET("LEFT_TREEVIEW");
	
	if (!g_node_first_child(MW_GET("LEFT_NODE_ROOT"))) return;
	// If entry is empty -> scroll to top.
	if (!(GTK_ENTRY(searchEntry)->text_length)){
		path = gtk_tree_path_new_root();
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path ,NULL ,FALSE );
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path, NULL, TRUE, 0.0, 0.0);
		gtk_tree_path_free (path);
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)));
		old_pos = -1;
		return;
	}
	// search for the entry.
	entry_text = txtOut((gchar *)gtk_entry_get_text(GTK_ENTRY(searchEntry)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_tree_model_get_iter_first(model, &iter); 
	while(1){
		gtk_tree_model_get(model, &iter, LEFT_LIST__CHANNEL_NODE, &node_channel, -1);
		if (cmpStr(((channelEntry*)node_channel->data)->channelName, entry_text) >= 0) break;
		if (gtk_tree_model_iter_next(model, &iter) == FALSE) break;
		pos++;
	}
	if (old_pos != pos){
		sprintf(path_string, "%d", pos);
		path = gtk_tree_path_new_from_string(path_string);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path, NULL, TRUE, 0.5, 0);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path ,NULL ,FALSE );
		gtk_tree_path_free(path);
		old_pos = pos;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Zap to channel.
////////////////////////////////////////////////////////////////////////////////
gpointer threadZapping(gpointer data){
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
	GtkTreeSelection* selection = gtk_tree_view_get_selection(MW_GET("RIGHT_CHANNEL_TREEVIEW"));
	GtkTreeIter iter;
	GNode *zap_node;
	gint posRadio = 0, posTV = 0, bouquet;
	gint zapDelay, waitLogin = 0;
	gchar txtBuffer[50];

	zapDelay = atoi(cfg_read("Timers","zapping delay" ,"5000"));
	zap_node = g_node_first_child (MW_GET("AKT_BOUQUET"));
	bouquet = g_node_child_position(MW_GET("RIGHT_NODE_ROOT"), MW_GET("AKT_BOUQUET")) +1;  
	gtk_tree_model_get_iter_first(model, &iter);
	do{
		channelEntry *channel = (channelEntry*) zap_node->data;
		if (channel->serviceType == DATA) continue;
		if (channel->serviceType == RADIO) posRadio++; else posTV++;
		if (gtk_tree_selection_iter_is_selected(selection, &iter)){
			if (channel->serviceType != RADIO) sprintf(txtBuffer,"pzapit %d %d\n"    , bouquet, posTV);
			else                               sprintf(txtBuffer,"pzapit -ra %d %d\n", bouquet, posRadio);
			while (!t_login && waitLogin++ < 6) g_usleep(1000000); // Wait for telnet to connect.
			if (waitLogin >5) return 0;
			sendTelnet(txtBuffer);
			g_usleep(zapDelay*1000); // wait x ms
		}
		zap_node = zap_node->next;
	}while (gtk_tree_model_iter_next(model, &iter));
	zapActive =0;
	return 0;
}

void zapToChannel(){
	if (zapActive) return;
	if (!g_node_first_child(MW_GET("AKT_BOUQUET"))) return;
	if (!(GTK_WIDGET_VISIBLE(MW_GET("CHANNEL_SCROLLER")))) return;
	if (!telnetConnected) but_telnet_dialog(NULL, NULL);
	zapActive = 1;
	g_thread_create(threadZapping, NULL, FALSE, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Prepare the zapping.
////////////////////////////////////////////////////////////////////////////////
gpointer threadPrepZap(gpointer data){
	gint waitLogin = 0;
	while (!t_login && waitLogin++ < 6) g_usleep(1000000); // Wait for telnet to connect.
	if (waitLogin >5) return 0;
	sendTelnet("pzapit -c\n");
	preActive = 0;
	return 0;
}

void prepareZap(){
	if (preActive) return;
	but_ftp_upload();
	if (!telnetConnected) but_telnet_dialog(NULL, NULL);
	preActive = 1;
	g_thread_create(threadPrepZap, NULL, FALSE, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Key events.
////////////////////////////////////////////////////////////////////////////////
gboolean key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data){
	if (MW_GET("TELNET_VIEW") && GTK_WIDGET_HAS_FOCUS(MW_GET("TELNET_VIEW"))) return FALSE;
	switch (event->keyval){
		case GDK_minus:
			but_move(NULL, (gpointer) 0);
			return TRUE;
		case GDK_plus:
			but_move(NULL, (gpointer) 1);
			return TRUE;
		case GDK_F1:
			but_help();
			return TRUE;
		case GDK_F4:
			but_copy_channels(NULL,(gpointer)1);
			return TRUE;
		case GDK_F5:
			but_copy_channels(NULL,(gpointer)0);
			return TRUE;
		case GDK_Return:
		case GDK_KP_Enter:
			if (!GTK_WIDGET_HAS_FOCUS(MW_GET("SEARCH_CHANNEL_ENTRY"))) break;
			but_copy_channels(NULL,(gpointer)0);
			return TRUE;
		case GDK_F8:
		case GDK_Delete:
		case GDK_BackSpace:
			if ((!GTK_WIDGET_HAS_FOCUS(MW_GET("RIGHT_CHANNEL_TREEVIEW")))
			&&  (!GTK_WIDGET_HAS_FOCUS(MW_GET("RIGHT_BOUQUET_TREEVIEW")))) break;
			but_del_entries(NULL,NULL);
			return TRUE;
		case GDK_F12:
			zapToChannel();
			return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// swap a row in right list_store
////////////////////////////////////////////////////////////////////////////////
void swap_row(gint view, gint pos1, gint pos2){
	GtkTreeIter iter;
	GtkTreeSelection* selection;
	GtkTreeModel *model;
	GNode *move1, *move2;
	if (view == VIEW_CHANNELS){
		channelEntry *channel;
		GdkPixbuf **serviceType = MW_GET("SERVICE_TYPE_GFX");
		// swap: node[pos1] <-> node[pos2]
		move1 = g_node_nth_child(MW_GET("AKT_BOUQUET"), pos1);
		move2 = g_node_nth_child(MW_GET("AKT_BOUQUET"), pos2);
		channel = move1->data;
		move1->data = move2->data;
		move2->data = channel;
		// swap: list_entry[pos1] <-> list_entry[pos2]
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
		channel = move1->data;
		gtk_tree_model_iter_nth_child(model, &iter, NULL, pos1);
		gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter,
			RIGHT_CHANNEL_LIST__DISEQC , channel->diseqc,
			RIGHT_CHANNEL_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
			RIGHT_CHANNEL_LIST__CHANNEL, txtIn(channel->channelName), -1);
		channel = move2->data;
		gtk_tree_model_iter_nth_child(model, &iter, NULL, pos2);
		gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter,
			RIGHT_CHANNEL_LIST__DISEQC , channel->diseqc,
			RIGHT_CHANNEL_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
			RIGHT_CHANNEL_LIST__CHANNEL, txtIn(channel->channelName), -1);
		countChannels();
	}else{
		GNode *child1, *child2;
		bouquetEntry *bouquet;
		// swap: node[pos1] <-> node[pos2]
		move1 = g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"), pos1);
		child1= g_node_first_child(move1);
		move2 = g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"), pos2);
		child2= g_node_first_child(move2);
		bouquet         = move1->data;
		move1->data     = move2->data;
		move2->data     = bouquet;
		move1->children = child2;
		move2->children = child1;
		while (child1) {child1->parent = move2; child1 = child1->next;}
		while (child2) {child2->parent = move1; child2 = child2->next;}
		// swap: list_entry[pos1] <-> list_entry[pos2]
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
		bouquet = move1->data;
		gtk_tree_model_iter_nth_child(model, &iter, NULL, pos1);
		gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter,
			RIGHT_BOUQUET_LIST__HIDDEN , bouquet->hidden,
			RIGHT_BOUQUET_LIST__LOCKED , bouquet->locked,
			RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquet->bouquetName), -1);
		bouquet = move2->data;
		gtk_tree_model_iter_nth_child(model, &iter, NULL, pos2);
		gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter,
			RIGHT_BOUQUET_LIST__HIDDEN , bouquet->hidden,
			RIGHT_BOUQUET_LIST__LOCKED , bouquet->locked,
			RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquet->bouquetName), -1);
	}
 	gtk_tree_model_iter_nth_child(model, &iter, NULL, pos2);
	gtk_tree_selection_unselect_iter(selection, &iter);
 	gtk_tree_model_iter_nth_child(model, &iter, NULL, pos1);
	gtk_tree_selection_select_iter(selection, &iter);
}

////////////////////////////////////////////////////////////////////////////////
// move up/down a channel.
////////////////////////////////////////////////////////////////////////////////
void but_move(gpointer callback_data, gpointer direction){
	GtkWidget* treeview;
	GtkTreeModel* model;
	GtkTreeSelection* selection;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gint view;
	gint i = 0;
	gint sumEntries;

	if (!g_node_first_child(MW_GET("RIGHT_NODE_ROOT"))) return;
	if (GTK_WIDGET_VISIBLE(MW_GET("CHANNEL_SCROLLER"))){
		if (!g_node_first_child(MW_GET("AKT_BOUQUET"))) return;
		treeview = MW_GET("RIGHT_CHANNEL_TREEVIEW");
		view = VIEW_CHANNELS;
		sumEntries = g_node_n_children(MW_GET("AKT_BOUQUET"))-1;
	}else{
		treeview = MW_GET("RIGHT_BOUQUET_TREEVIEW");
		view = VIEW_BOUQUETS;
		sumEntries = g_node_n_children(MW_GET("RIGHT_NODE_ROOT"))-1;
	}
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	undoJob++;
	if (((gint)direction &1)){ // move channels down.
		gtk_tree_model_iter_nth_child(model, &iter, NULL, sumEntries);
		if (gtk_tree_selection_iter_is_selected(selection, &iter)) return;
		while (sumEntries--){
			gtk_tree_model_iter_nth_child(model, &iter, NULL, sumEntries);
			if (gtk_tree_selection_iter_is_selected(selection, &iter)){
				if (!path) path = gtk_tree_model_get_path(model, &iter);
				pushUndo(view, UNDO_SWAP, sumEntries+1, (gpointer)sumEntries, FALSE);
				swap_row(view, sumEntries+1, sumEntries);
			}
		}
	}else{ // move channels up.
		gtk_tree_model_get_iter_first(model, &iter);
		if (gtk_tree_selection_iter_is_selected(selection, &iter)) return;
		while (i++ < sumEntries){
			gtk_tree_model_iter_nth_child(model, &iter, NULL, i);
			if (gtk_tree_selection_iter_is_selected(selection, &iter)){
				if (!path) path = gtk_tree_model_get_path(model, &iter);
				pushUndo(view, UNDO_SWAP, i-1, (gpointer)i, FALSE);
				swap_row(view, i-1, i);
			}
		}
	}
	if (path){
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path, NULL, TRUE, 0, 0);
		gtk_tree_path_free(path);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Delete the whole GNode.
////////////////////////////////////////////////////////////////////////////////
void clear_node(GNode *root_node){
	GNode *node_parent;
	GNode *node_child;
	GNode *node_free;

	node_parent = g_node_first_child (root_node);
	while (node_parent){
		node_child = g_node_first_child (node_parent);
		while (node_child){
			node_free = node_child;
			free(node_child->data);
			node_child = node_child ->next;
			g_node_destroy(node_free);
		}
		node_free = node_parent;
		free(node_parent->data);
		node_parent = node_parent->next;
		g_node_destroy(node_free);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Delete a GNode.
////////////////////////////////////////////////////////////////////////////////
GNode *remove_node(GNode *node){
	GNode *parent_node = node->parent;
	free(node->data);
	g_node_destroy(node);
	return g_node_first_child(parent_node);
}

////////////////////////////////////////////////////////////////////////////////
// Copy a GNode.
////////////////////////////////////////////////////////////////////////////////
GNode *copy_channel_node(GNode *node_dest, GNode *node_dest_parent, GNode *node_src){
	node_dest = g_node_insert_before(node_dest_parent, node_dest,
 		g_node_new(malloc(sizeof(channelEntry))));
	memcpy(node_dest->data, node_src->data, sizeof(channelEntry));
	return node_dest;
}

////////////////////////////////////////////////////////////////////////////////
// sort the actual column in right listview.
////////////////////////////////////////////////////////////////////////////////
gint treeview_sort(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,gpointer column){
	gint col = (gint) column;

	if (col == LEFT_LIST__BOUQUET || col == LEFT_LIST__CHANNEL || col == RIGHT_CHANNEL_LIST__CHANNEL){
		gchar *p1, *p2;
		gtk_tree_model_get(model,a,col,&p1,-1);
		gtk_tree_model_get(model,b,col,&p2,-1);
//		return cmpStr(p1,p2);
		return cmpStr(txtOut(p1),txtOut(p2));
	}

	if (col == LEFT_LIST__DISEQC || col == RIGHT_CHANNEL_LIST__DISEQC){
		gint p1, p2;
		gtk_tree_model_get(model,a,col,&p1,-1);
		gtk_tree_model_get(model,b,col,&p2,-1);
		return p1 - p2;
	}

	if (col == LEFT_LIST__SERVICE || col == RIGHT_CHANNEL_LIST__SERVICE){
		gint p1, p2;
		gtk_tree_model_get(model,a,col,&p1,-1);
		gtk_tree_model_get(model,b,col,&p2,-1);
		return p1 - p2;
	}
	else return 0;
}

////////////////////////////////////////////////////////////////////////////////
// switch between channel/bouquet view.
////////////////////////////////////////////////////////////////////////////////
void but_toggleViewMode(GtkWidget *view, gboolean fromUndo){
	if (view == CHANNEL_VIEW) {
		gtk_widget_set_sensitive(MW_GET("BUT_ADD_BOUQUET"), FALSE);
		gtk_widget_set_sensitive(MW_GET("TOGGLE_VIEW"),TRUE);
		gtk_widget_set_sensitive(MW_GET("BUT_COPYALL"),TRUE);
		gtk_widget_set_sensitive(MW_GET("BUT_INSERT"),TRUE);
		gtk_widget_set_sensitive(MW_GET("BUT_COPY"),TRUE);
		gtk_widget_hide(MW_GET("BOUQUET_SCROLLER"));
		gtk_widget_show(MW_GET("CHANNEL_SCROLLER"));
		fill_channel_listview(MW_GET("AKT_BOUQUET"));
		if (!fromUndo) pushUndo(VIEW_SETTINGS, UNDO_CHANNEL_VIEW, 0, NULL, FALSE);
	}else{
		if (!g_node_first_child(MW_GET("RIGHT_NODE_ROOT"))) return;
		if (view == MW_GET("TOGGLE_VIEW")) undoJob++;
		gtk_widget_set_sensitive(MW_GET("BUT_ADD_BOUQUET"), TRUE);
		gtk_widget_set_sensitive(MW_GET("TOGGLE_VIEW"), FALSE);
		gtk_widget_set_sensitive(MW_GET("BUT_COPYALL"),FALSE);
		gtk_widget_set_sensitive(MW_GET("BUT_INSERT"),FALSE);
		gtk_widget_set_sensitive(MW_GET("BUT_COPY"),FALSE);
		gtk_widget_show(MW_GET("BOUQUET_SCROLLER"));
		gtk_widget_hide(MW_GET("CHANNEL_SCROLLER"));
		gtk_label_set_text(MW_GET("TOGGLE_LABEL"), _("<all Bouquets>"));
		updateStatusbar(NULL);
		if (!fromUndo) pushUndo(VIEW_SETTINGS, UNDO_BOUQUET_VIEW,
			g_node_child_position(MW_GET("RIGHT_NODE_ROOT"), MW_GET("AKT_BOUQUET")), NULL, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Switch Header on/off.
////////////////////////////////////////////////////////////////////////////////
void but_showHeader(gpointer button, GtkWidget *widget){
	gtk_tree_view_set_headers_visible(MW_GET("LEFT_TREEVIEW"), GTK_TOGGLE_BUTTON(button)->active);
	gtk_tree_view_set_headers_visible(MW_GET("RIGHT_CHANNEL_TREEVIEW"),
		GTK_TOGGLE_BUTTON(button)->active);
	gtk_tree_view_set_headers_visible(MW_GET("RIGHT_BOUQUET_TREEVIEW"),
		GTK_TOGGLE_BUTTON(button)->active);
}

////////////////////////////////////////////////////////////////////////////////
// Switch diseqc-column on/off.
////////////////////////////////////////////////////////////////////////////////
void but_showItems(gpointer button, GtkWidget *widget){
	gtk_tree_view_column_set_visible(MW_GET("LEFT_VIEW_DISEQC"), GTK_TOGGLE_BUTTON(button)->active);
	gtk_tree_view_column_set_visible(MW_GET("RIGHT_VIEW_DISEQC"),GTK_TOGGLE_BUTTON(button)->active);
}

////////////////////////////////////////////////////////////////////////////////
// Switch count-column on/off.
////////////////////////////////////////////////////////////////////////////////
void but_showCount(gpointer button, GtkWidget *widget){
	gtk_tree_view_column_set_visible(MW_GET("RIGHT_VIEW_NUMBERS"),GTK_TOGGLE_BUTTON(button)->active);
}

////////////////////////////////////////////////////////////////////////////////
// Switch icontext on/off.
////////////////////////////////////////////////////////////////////////////////
void but_showIconText(gpointer callback_data, gpointer button){
	gint style;
	switch ((gint) button){
		case 0:
			style = (gtk_toolbar_get_style(GTK_TOOLBAR(MW_GET("LEFT_TOOLBAR")))+1)&3,
			gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("LEFT_TOOLBAR")), style);
			break;
		case 1:
			style = (gtk_toolbar_get_style(GTK_TOOLBAR(MW_GET("MID_TOOLBAR")))+1)&3,
			gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("MID_TOOLBAR")), style);
			break;
		case 2:
			style = (gtk_toolbar_get_style(GTK_TOOLBAR(MW_GET("RIGHT_TOOLBAR")))+1)&3,
			gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("RIGHT_TOOLBAR")), style);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Clear left listview.
////////////////////////////////////////////////////////////////////////////////
void clear_left_listview(){
	clear_node(MW_GET("LEFT_NODE_ROOT"));
	gtk_list_store_clear(MW_GET("LEFT_LIST_STORE"));
}

////////////////////////////////////////////////////////////////////////////////
// Clear right listview.
////////////////////////////////////////////////////////////////////////////////
void clear_right_listview(){
	gint i;
	GNode *node_bouquet, *node_channel;
	statusbar *status = MW_GET("RIGHT_STATUSBAR");
	if (!g_node_first_child(MW_GET("RIGHT_NODE_ROOT"))) return; // kein Bouquet vorhanden.

	undoJob++;
	if (GTK_WIDGET_VISIBLE(MW_GET("BOUQUET_SCROLLER"))){
		pushUndo(VIEW_SETTINGS, UNDO_CHANNEL_VIEW, 0, NULL, TRUE);
		but_toggleViewMode(CHANNEL_VIEW, TRUE);
	}
 	else pushUndo(VIEW_SETTINGS, UNDO_BOUQUET_VIEW,
			g_node_child_position(MW_GET("RIGHT_NODE_ROOT"), MW_GET("AKT_BOUQUET")), NULL, FALSE);
	node_bouquet = g_node_first_child(MW_GET("RIGHT_NODE_ROOT"));
	while (node_bouquet){
	node_channel = g_node_first_child(node_bouquet);
		while (node_channel) {
			pushUndo(VIEW_CHANNELS, UNDO_DELETE, 0, node_channel, FALSE);
			node_channel = remove_node(node_channel);
		}
		pushUndo(VIEW_BOUQUETS, UNDO_DELETE, 0, node_bouquet, FALSE);
		node_bouquet = remove_node(node_bouquet);
	}
	MW_SET("AKT_BOUQUET", NULL);
	gtk_label_set_text(MW_GET("TOGGLE_LABEL"), _("<Bouquets are empty>"));
	gtk_list_store_clear(MW_GET("RIGHT_LIST_CHANNEL_STORE"));
	gtk_list_store_clear(MW_GET("RIGHT_LIST_BOUQUET_STORE"));
	for (i=0; i< N_SERVICES; i++) status->sumChannels[i]=0;
	updateStatusbar(NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Update statusbar
////////////////////////////////////////////////////////////////////////////////
void updateStatusbar(statusbar *status){
	gchar out[100];
	if (status == MW_GET("LEFT_STATUSBAR")){
		sprintf(out, _(" %d / %d / %d Channels loaded into %d Bouquets"),
			status->sumChannels[TV] + status->sumChannels[NVOD], status->sumChannels[RADIO],
			status->sumChannels[DATA], status->sumBouquets );
		gtk_statusbar_pop(GTK_STATUSBAR(status->widget), 0);
		gtk_statusbar_push(GTK_STATUSBAR(status->widget),0, out);
	}else{ // Right Statusbar.
		status = MW_GET("RIGHT_STATUSBAR");
		gtk_statusbar_pop(GTK_STATUSBAR(status->widget), 0);
		if (GTK_WIDGET_VISIBLE(MW_GET("CHANNEL_SCROLLER"))){
			if (MW_GET("AKT_BOUQUET")){
				sprintf(out, _(" %d / %d / %d Channels in this Bouquet"),
					status->sumChannels[TV] + status->sumChannels[NVOD], status->sumChannels[RADIO],
					status->sumChannels[DATA] );
				gtk_statusbar_push(GTK_STATUSBAR(status->widget),0, out);
			}else gtk_statusbar_push(GTK_STATUSBAR(status->widget),0, "");
		}else{
			gtk_statusbar_push(GTK_STATUSBAR(status->widget),0, _(" dblclk on Bouquet for Channel-View"));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Add a Bouquet
////////////////////////////////////////////////////////////////////////////////
GNode *addBouquet(gchar *bouquetName){
	bouquetEntry *bouquet = malloc(sizeof(bouquetEntry));
	GNode *node_bouquet;
	GtkTreeModel*model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
	GtkTreeIter iter1;

	pushUndo(VIEW_BOUQUETS, UNDO_COPY, 0, NULL, FALSE);
	node_bouquet = g_node_prepend(MW_GET("RIGHT_NODE_ROOT"), g_node_new(bouquet));
	strcpy(bouquet->bouquetName, bouquetName);
	bouquet->hidden = 0;
	bouquet->locked = 0;
	gtk_list_store_prepend(GTK_LIST_STORE(model), &iter1);
	gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1,
		RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquetName),
		RIGHT_BOUQUET_LIST__HIDDEN, FALSE,
		RIGHT_BOUQUET_LIST__LOCKED, FALSE,
		RIGHT_BOUQUET_LIST__EDITABLE, TRUE,
		RIGHT_BOUQUET_LIST__NODE, node_bouquet, -1);
	return  node_bouquet;
}

void but_addBouquet(gpointer button, GtkWidget *widget){
	undoJob++;
	addBouquet(_("- new one -"));
}

////////////////////////////////////////////////////////////////////////////////
// Fill left Listview.
////////////////////////////////////////////////////////////////////////////////
void fill_left_listview(gint sumBouquets){
	GtkTreeIter iter;
	GdkPixbuf **serviceType = MW_GET("SERVICE_TYPE_GFX");
	GtkListStore *store     = MW_GET("LEFT_LIST_STORE");
	GNode *node_bouquet;
	GNode *node_channel;
	bouquetEntry *bouquet;
	channelEntry *channel;
	gint i;
	statusbar *status = MW_GET("LEFT_STATUSBAR");

	status->sumBouquets = 0;
	for (i=0; i< N_SERVICES; i++) status->sumChannels[i]=0;
	node_bouquet = g_node_first_child(MW_GET("LEFT_NODE_ROOT"));
	gtk_list_store_clear(MW_GET("LEFT_LIST_STORE"));
	while (node_bouquet){
		status->sumBouquets++;
		bouquet = (bouquetEntry *) node_bouquet->data;
		node_channel = g_node_first_child (node_bouquet);
		while (node_channel){
			channel = (channelEntry *) node_channel->data;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
				LEFT_LIST__DISEQC , channel->diseqc,
				LEFT_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
				LEFT_LIST__BOUQUET, txtIn(bouquet->bouquetName),
				LEFT_LIST__CHANNEL, txtIn(channel->channelName),
				LEFT_LIST__CHANNEL_NODE, node_channel,
				-1);
			status->sumChannels[getServicePic(channel->serviceType)]++;
			node_channel = node_channel ->next;
		}
		node_bouquet = node_bouquet->next;
	}
	updateStatusbar(status);
}

////////////////////////////////////////////////////////////////////////////////
// Fill right channel-listview.
////////////////////////////////////////////////////////////////////////////////
void fill_channel_listview(GNode *node_bouquet){
	GtkTreeIter iter;
	GdkPixbuf **serviceType = MW_GET("SERVICE_TYPE_GFX");
	GtkListStore *store     = MW_GET("RIGHT_LIST_CHANNEL_STORE");
	GNode *node_channel;
	bouquetEntry *bouquet;
	channelEntry *channel;
	gint i;
	statusbar *status = MW_GET("RIGHT_STATUSBAR");

	gtk_list_store_clear(MW_GET("RIGHT_LIST_CHANNEL_STORE"));
	if (!node_bouquet) return;
	for (i=0; i< N_SERVICES; i++) status->sumChannels[i]=0;
	bouquet = (bouquetEntry *) node_bouquet->data;
	node_channel = g_node_first_child (node_bouquet);
	while (node_channel){
		channel = (channelEntry *) node_channel->data;
  gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			RIGHT_CHANNEL_LIST__DISEQC , channel->diseqc,
			RIGHT_CHANNEL_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
			RIGHT_CHANNEL_LIST__CHANNEL, txtIn(channel->channelName),
			RIGHT_CHANNEL_LIST__EDITABLE, TRUE,
			RIGHT_CHANNEL_LIST__NODE, node_channel,
			-1);
  status->sumChannels[getServicePic(channel->serviceType)]++;
		node_channel = node_channel ->next;
	}
	gtk_label_set_text(MW_GET("TOGGLE_LABEL"), txtIn(bouquet->bouquetName));
	countChannels();
	updateStatusbar(status);
}

////////////////////////////////////////////////////////////////////////////////
// Fill right bouquet-listview.
////////////////////////////////////////////////////////////////////////////////
void fill_bouquet_listview(){
	GtkTreeIter iter;
	bouquetEntry *bouquet;
	GtkListStore *store = MW_GET("RIGHT_LIST_BOUQUET_STORE");
	GNode *node_bouquet = g_node_first_child(MW_GET("RIGHT_NODE_ROOT"));
	gtk_list_store_clear(store);
	while (node_bouquet){
		bouquet = (bouquetEntry *) node_bouquet->data;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			RIGHT_BOUQUET_LIST__HIDDEN,  bouquet->hidden,
			RIGHT_BOUQUET_LIST__LOCKED,  bouquet->locked,
			RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquet->bouquetName),
			RIGHT_BOUQUET_LIST__EDITABLE, TRUE,
			RIGHT_BOUQUET_LIST__NODE, node_bouquet,
			-1);
		node_bouquet = node_bouquet->next;
	}
}

////////////////////////////////////////////////////////////////////////////////
// copy selected Channels
////////////////////////////////////////////////////////////////////////////////
void get_left_selection(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer insert){
	GNode *src_node;
	gchar *pathStr;
	gtk_tree_model_get(model, iter, LEFT_LIST__CHANNEL_NODE, &src_node,-1);

	if (insert){
		GtkTreeModel *dest_model;
		GtkTreePath *dest_path;
		GtkTreeIter dest_iter;
		GNode *dest_node;
		dest_model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")), &dest_path, NULL);
		if (!dest_path)  return; // no cursor was set.
		pathStr = gtk_tree_path_to_string(dest_path);
		pushUndo(VIEW_CHANNELS, UNDO_COPY, atoi(pathStr), MW_GET("AKT_BOUQUET"), FALSE);
		g_free(pathStr);
		gtk_tree_model_get_iter(dest_model, &dest_iter, dest_path);
		gtk_tree_model_get(dest_model, &dest_iter, RIGHT_CHANNEL_LIST__NODE, &dest_node,-1);
		copy_channel_node(dest_node, MW_GET("AKT_BOUQUET"), src_node);
	}else{ // copy
		copy_channel_node(NULL, MW_GET("AKT_BOUQUET"), src_node);
		pushUndo(VIEW_CHANNELS, UNDO_COPY, -1, MW_GET("AKT_BOUQUET"), FALSE);
	}
}

void but_copy_channels(gpointer callback_data, gpointer action){
	GtkTreePath *path;
	if (action && !MW_GET("AKT_BOUQUET")) return;
	undoJob++;
	//  if no Bouquet exist -> make one.
	if (!MW_GET("AKT_BOUQUET"))	MW_SET("AKT_BOUQUET", addBouquet(_("<my Bouquet>")));
	gtk_tree_selection_selected_foreach (
		gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("LEFT_TREEVIEW"))),
		(GtkTreeSelectionForeachFunc)get_left_selection, action);
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")), &path, NULL);
	fill_channel_listview(MW_GET("AKT_BOUQUET"));
	if (path)	gtk_tree_view_set_cursor(MW_GET("RIGHT_CHANNEL_TREEVIEW"), path, NULL, FALSE);
}

////////////////////////////////////////////////////////////////////////////////
// Copy all channels into right Listview.
////////////////////////////////////////////////////////////////////////////////
void but_copyAll(gpointer callback_data, GtkWidget *widget){
	GNode *node_left_channel, *node_right_bouquet, *node_right_channel;
	bouquetEntry *right_bouquet;
	channelEntry *right_channel;
	gint pos = g_node_n_children(MW_GET("RIGHT_NODE_ROOT"));
	GNode *node_left_bouquet  = g_node_first_child(MW_GET("LEFT_NODE_ROOT"));
	if (!node_left_bouquet) return;
	undoJob++;
	while (node_left_bouquet){
		right_bouquet = malloc(sizeof(bouquetEntry));
  	memcpy(right_bouquet, (bouquetEntry *) node_left_bouquet->data, sizeof(bouquetEntry));
		node_right_bouquet = g_node_append(MW_GET("RIGHT_NODE_ROOT"), g_node_new(right_bouquet));
		pushUndo(VIEW_BOUQUETS, UNDO_COPY, pos++, NULL, FALSE);
		node_left_channel = g_node_first_child(node_left_bouquet);
		while (node_left_channel){
			right_channel = malloc(sizeof(channelEntry));
   		memcpy(right_channel,(channelEntry *) node_left_channel->data, sizeof(channelEntry));
			node_right_channel = g_node_append(node_right_bouquet, g_node_new(right_channel));
			pushUndo(VIEW_CHANNELS, UNDO_COPY, -1, node_right_bouquet, FALSE);
			node_left_channel = node_left_channel->next;
		}
		node_left_bouquet = node_left_bouquet->next;
	}
	MW_SET("AKT_BOUQUET", g_node_first_child(MW_GET("RIGHT_NODE_ROOT")));
	fill_bouquet_listview();
	but_toggleViewMode(CHANNEL_VIEW, TRUE);
	pushUndo(VIEW_SETTINGS, UNDO_BOUQUET_VIEW, -1, NULL, TRUE);
}

////////////////////////////////////////////////////////////////////////////////
// Toggle hidden/locked flag.
////////////////////////////////////////////////////////////////////////////////
gint column_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data){
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	gboolean toggle_item;
	gint *column;
	GNode *node;
	bouquetEntry *bouquet;

	column = g_object_get_data (G_OBJECT (cell), "column");
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, column, &toggle_item, -1);
	toggle_item ^= 1;
	gtk_list_store_set(GTK_LIST_STORE (model), &iter, column, toggle_item, -1);
	gtk_tree_model_get(model, &iter, RIGHT_BOUQUET_LIST__NODE, &node, -1);
	undoJob++;
	pushUndo(VIEW_BOUQUETS, UNDO_HIDE_LOCK, atoi(path_str), node, FALSE);
	bouquet = (bouquetEntry *) node->data;
	if (column == RIGHT_BOUQUET_LIST__HIDDEN) bouquet->hidden = toggle_item;
	else bouquet->locked = toggle_item;
	gtk_tree_path_free (path);
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Channel/Bouquet was renamed.
////////////////////////////////////////////////////////////////////////////////
gint cell_edited(GtkCellRendererText *cell, gchar *path_str, gchar *new_text, gpointer data){
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	GNode *node;
	bouquetEntry *bouquet;
	channelEntry *channel;

	undoJob++;
	gtk_tree_model_get_iter (model, &iter, path);
	switch ((gint)g_object_get_data(G_OBJECT (cell), "viewmode")){
		case 0:
			gtk_list_store_set(GTK_LIST_STORE (model), &iter, RIGHT_BOUQUET_LIST__BOUQUET, new_text, -1);
			gtk_tree_model_get(model, &iter, RIGHT_BOUQUET_LIST__NODE, &node, -1);
			bouquet = node->data;
			if (strcmp(bouquet->bouquetName, txtOut(new_text))) {
				pushUndo(VIEW_BOUQUETS, UNDO_RENAME, atoi(path_str), node, FALSE);
				strncpy(bouquet->bouquetName, txtOut(new_text),MAX_TXT_LEN);
			}
		break;
		case 1:
			gtk_list_store_set(GTK_LIST_STORE (model), &iter, RIGHT_CHANNEL_LIST__CHANNEL, new_text, -1);
			gtk_tree_model_get(model, &iter, RIGHT_CHANNEL_LIST__NODE, &node, -1);
			channel = node->data;
			if (strcmp(channel->channelName, txtOut(new_text))) {
				pushUndo(VIEW_CHANNELS, UNDO_RENAME, atoi(path_str), node, FALSE);
				strncpy(channel->channelName, txtOut(new_text),MAX_TXT_LEN);
			}
		break;
	}
	gtk_tree_path_free(path);
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Delete selected channels/bouquets.
////////////////////////////////////////////////////////////////////////////////
void but_del_entries(gpointer callback_data, GtkWidget *widget){
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkTreeSelection* selection;
	GtkTreeIter iter;
	gint view, column, firstSel = 0, pos = 0;
	GNode *node_channel, *node = g_node_first_child(MW_GET("RIGHT_NODE_ROOT"));
	statusbar *status = MW_GET("RIGHT_STATUSBAR");

	if (!node) return;
	if (GTK_WIDGET_VISIBLE(MW_GET("CHANNEL_SCROLLER"))){
		if (!node->children) return;
		treeview = MW_GET("RIGHT_CHANNEL_TREEVIEW");
		view = VIEW_CHANNELS;
		column = RIGHT_CHANNEL_LIST__NODE;
	}else{
		treeview = MW_GET("RIGHT_BOUQUET_TREEVIEW");
		view = VIEW_BOUQUETS;
		column = RIGHT_BOUQUET_LIST__NODE;
	}
	undoJob++;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	while (gtk_tree_model_iter_nth_child(model, &iter, NULL, pos)){
		if (gtk_tree_selection_iter_is_selected(selection, &iter)){
			gtk_tree_model_get(model, &iter, column, &node, -1);
			if (view == VIEW_CHANNELS)
   			status->sumChannels[getServicePic(((channelEntry *)node->data)->serviceType)]--;
			else {   // VIEW_BOUQUETS
				node_channel = g_node_first_child(node);
				while (node_channel) {
					pushUndo(VIEW_CHANNELS, UNDO_DELETE, 0, node_channel, FALSE);
					node_channel = remove_node(node_channel);
				}
			}
			pushUndo(view, UNDO_DELETE, pos, node, FALSE);
			remove_node(node);
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			firstSel = pos;
		}else	pos++;
	}
	updateStatusbar(status);
	if (!gtk_tree_model_iter_nth_child(model, &iter, NULL, firstSel))
		if (firstSel--) gtk_tree_model_iter_nth_child(model, &iter, NULL, firstSel);
	if (firstSel >=0) gtk_tree_selection_select_iter(selection, &iter);

	if (!g_node_first_child(MW_GET("RIGHT_NODE_ROOT"))){
		gtk_list_store_clear(MW_GET("RIGHT_LIST_CHANNEL_STORE"));
		gtk_list_store_clear(MW_GET("RIGHT_LIST_CHANNEL_STORE"));
		but_toggleViewMode(BOUQUET_VIEW, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Bouquet selected.
////////////////////////////////////////////////////////////////////////////////
gboolean dblClick(GtkTreeView *treeview, GdkEventButton *event, gpointer data){
	GNode *node = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;

	if (event->type!=GDK_2BUTTON_PRESS) return FALSE;
	if (gtk_tree_view_get_path_at_pos(treeview, event->x , event->y, &path, NULL, NULL, NULL)){
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, RIGHT_BOUQUET_LIST__NODE,  &node, -1);
		MW_SET("AKT_BOUQUET", node);
		undoJob++;
		but_toggleViewMode(CHANNEL_VIEW, FALSE);
	}
	gtk_tree_path_free (path);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Copy drag-data to clipboard.
////////////////////////////////////////////////////////////////////////////////
void tree_drag_data_get(GtkWidget *widget, GdkDragContext *dc,
	GtkSelectionData *selectionData,guint info,guint t,gpointer data){
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList *clipboard_list = g_list_first(MW_GET("CLIPBOARD_LIST"));
	clipboard *clipboard_entry;
	gint action, source, pos=0;

	switch ((gint) data){
		case 0: // soure == left-treeview.
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("LEFT_TREEVIEW")));
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("LEFT_TREEVIEW")));
			action = CLIPBOARD_COPY;
			source = 0;
			break;

		case 1: // soure == right-treeview (channel-view).
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
			action = CLIPBOARD_MOVE;
			source = 1;
			break;

		case 2: // soure == left-treeview (bouuqet-view).
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
			action = CLIPBOARD_MOVE;
			source = 2;
		break;
	}

	gtk_tree_model_get_iter_root(model, &iter);
	do{
		if (gtk_tree_selection_iter_is_selected(selection, &iter)) {
			clipboard_entry = malloc(sizeof(clipboard));
			clipboard_entry->pos = pos;
			clipboard_entry->action = action;
			clipboard_entry->source = source;
			clipboard_list = g_list_prepend(clipboard_list, clipboard_entry);
		}
		pos++;
	}while (gtk_tree_model_iter_next(model, &iter));
	MW_SET("CLIPBOARD_LIST", clipboard_list);
}

////////////////////////////////////////////////////////////////////////////////
// move a row in right list_store
////////////////////////////////////////////////////////////////////////////////
void move_row(gint view, gint pos1, gint pos2){
	GtkTreeIter iter;
	GtkTreeSelection* selection;
	GtkTreeModel *model;
	GNode *move1, *move2;

	if (pos1 > pos2) pos1++;
	if (view == VIEW_CHANNELS){
		channelEntry *channel;
		GdkPixbuf **serviceType = MW_GET("SERVICE_TYPE_GFX");
		move2 = g_node_nth_child(MW_GET("AKT_BOUQUET"), pos2);
		move1 = g_node_insert(MW_GET("AKT_BOUQUET"), pos1, g_node_copy(move2));
		g_node_destroy(move2);
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
		gtk_tree_model_iter_nth_child(model, &iter, NULL, pos2);
		gtk_list_store_remove(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter);
		channel = move1->data;
		if (pos1 > pos2) pos1--;
		gtk_list_store_insert(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter, pos1);
		gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter,
			RIGHT_CHANNEL_LIST__DISEQC , channel->diseqc,
			RIGHT_CHANNEL_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
			RIGHT_CHANNEL_LIST__CHANNEL, txtIn(channel->channelName),
			RIGHT_CHANNEL_LIST__EDITABLE, TRUE,
			RIGHT_CHANNEL_LIST__NODE, move1, -1);
		countChannels();
	}else{
		bouquetEntry *bouquet;
		move2 = g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"), pos2);
		move1 = g_node_insert(MW_GET("RIGHT_NODE_ROOT"),pos1, g_node_copy(move2));
		g_node_destroy(move2);
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
		gtk_tree_model_iter_nth_child(model, &iter, NULL, pos2);
		gtk_list_store_remove(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter);
		bouquet = move1->data;
		if (pos1 > pos2) pos1--;
		gtk_list_store_insert(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter, pos1);
		gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter,
			RIGHT_BOUQUET_LIST__HIDDEN , bouquet->hidden,
			RIGHT_BOUQUET_LIST__LOCKED , bouquet->locked,
			RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquet->bouquetName),
			RIGHT_BOUQUET_LIST__EDITABLE, TRUE,
			RIGHT_BOUQUET_LIST__NODE, move1, -1);
	}
 	gtk_tree_model_iter_nth_child(model, &iter, NULL, pos2);
	gtk_tree_selection_unselect_iter(selection, &iter);
 	gtk_tree_model_iter_nth_child(model, &iter, NULL, pos1);
	gtk_tree_selection_select_iter(selection, &iter);
}

////////////////////////////////////////////////////////////////////////////////
// Drop data from clipboard.
////////////////////////////////////////////////////////////////////////////////
void tree_drag_data_received(GtkWidget *widget,GdkDragContext *dc, gint x, gint y,
	GtkSelectionData *selection,guint info,guint t,gpointer data){
	GList *src_list = g_list_first(MW_GET("CLIPBOARD_LIST"));
	GtkTreeViewDropPosition type;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *pathStr;
	gint pathInt, pos, source;
	GNode *src_node;
	GtkTreeModel *model;
	
	undoJob++;
	pos =    ((clipboard*)src_list->data)->pos;
	source = ((clipboard*)src_list->data)->source;
	switch ((gint) data){
		case 1: // source was a channel.
			if (!gtk_tree_view_get_dest_row_at_pos(MW_GET("RIGHT_CHANNEL_TREEVIEW"), x , y, &path, &type))
				break;
			pathStr = gtk_tree_path_to_string(path);
			pathInt =  atoi(pathStr);
			g_free(pathStr);
			gtk_tree_path_free (path);
			if (!source){	// copy.
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("LEFT_TREEVIEW")));
				gtk_tree_model_iter_nth_child(model, &iter, NULL, pos);
				gtk_tree_model_get(model, &iter, LEFT_LIST__CHANNEL_NODE, &src_node, -1);
				pushUndo(VIEW_CHANNELS, UNDO_COPY, pathInt, MW_GET("AKT_BOUQUET"), FALSE);
				copy_channel_node(g_node_nth_child(MW_GET("AKT_BOUQUET"), pathInt), MW_GET("AKT_BOUQUET"), src_node);
			}else{				// move.
				pushUndo(VIEW_CHANNELS, UNDO_MOVE, pathInt, (gpointer)pos, FALSE);
				move_row(VIEW_CHANNELS, pathInt, pos);
			}
			fill_channel_listview(MW_GET("AKT_BOUQUET"));
			free(src_list->data);
			g_list_remove(src_list, src_list->data);
		break;

		case 2: // source was a bouquet.
			if (!gtk_tree_view_get_dest_row_at_pos(MW_GET("RIGHT_BOUQUET_TREEVIEW"), x , y, &path, &type))
				break;
			if (source == 2){
				pathStr = gtk_tree_path_to_string(path);
				pathInt =  atoi(pathStr);
				g_free(pathStr);
				gtk_tree_path_free (path);
				pushUndo(VIEW_BOUQUETS, UNDO_MOVE, pathInt, (gpointer)pos, FALSE);
				move_row(VIEW_BOUQUETS, pathInt, pos);
			}
			free(src_list->data);
			g_list_remove(src_list, src_list->data);
		break;
	}
	MW_SET("CLIPBOARD_LIST", NULL);
}

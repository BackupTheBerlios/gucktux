/***************************************************************************
                          tools.c  -  description
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

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tools.h"
#include "listen.h"
#include "gsatedit.h"

#define MAX_UNDO_STEPS 15

////////////////////////////////////////////////////////////////////////////////
// Only MAX_UNDO_STEPS are allowed.
////////////////////////////////////////////////////////////////////////////////
gint countUndo(gint delta){
	static gint steps =0;
	gint jobNr;
	undo  *undo_entry;
	GList *undo_list = MW_GET("UNDO_LIST");

	if (!delta) steps = 0;
	steps+= delta;
	if (steps > MAX_UNDO_STEPS) { // delete one Undo-step.
		do{
			undo_entry = g_list_last(undo_list)->data;
			jobNr = undo_entry->jobNr;
			if (undo_entry->channel) free(undo_entry->channel);
			if (undo_entry->bouquet) free(undo_entry->bouquet);
			undo_list = g_list_remove(undo_list, undo_entry);
			free(undo_entry);
		}while (jobNr == ((undo *)g_list_last(undo_list)->data)->jobNr);
		steps--;
	}
	MW_SET("UNDO_LIST", undo_list);
	if (!steps) gtk_widget_set_sensitive(MW_GET("BUT_UNDO"), FALSE);
	return steps;
}

////////////////////////////////////////////////////////////////////////////////
// Clear Undo Buffer.
////////////////////////////////////////////////////////////////////////////////
void clearUndo(){
	undo  *undo_entry;
	GList *undo_list = MW_GET("UNDO_LIST");

	while (undo_list){
		undo_entry = undo_list->data;
		if (undo_entry->channel) free(undo_entry->channel);
		if (undo_entry->bouquet) free(undo_entry->bouquet);
		undo_list = g_list_remove(undo_list, undo_entry);
		free(undo_entry);
	}
	countUndo(0);
	MW_SET("UNDO_LIST", NULL);
}

////////////////////////////////////////////////////////////////////////////////
// remember an undo step
////////////////////////////////////////////////////////////////////////////////
void pushUndo(gint view, gint action, gint pos, GNode *node, gboolean callFromRedo){
	static gint lastJob = -123456;
	GList *undo_list = MW_GET("UNDO_LIST");
	undo *undo_entry = malloc(sizeof(undo));
	undo_entry->pos     = pos;
	undo_entry->view    = view;
	undo_entry->jobNr   = undoJob;
	undo_entry->action  = action;
	undo_entry->node    = node;
	undo_entry->channel = NULL;
	undo_entry->bouquet = NULL;

	// every action (but Redo) clears Redo-Buffer.
	if (!callFromRedo) clearRedo(MW_GET("REDO_LIST"));
	switch (undo_entry->view){
		case VIEW_CHANNELS:
			switch (action){
				case UNDO_DELETE:
				case UNDO_RENAME:
					undo_entry->channel = malloc(sizeof(channelEntry));
					memcpy(undo_entry->channel, node->data, sizeof(channelEntry));
					undo_entry->node = NULL;
				break;
			}
		break; // END OF case VIEW_CHANNELS

		case VIEW_BOUQUETS:
			switch (action){
				case UNDO_DELETE:
				case UNDO_RENAME:
				case UNDO_HIDE_LOCK:
					undo_entry->bouquet = malloc(sizeof(bouquetEntry));
					memcpy(undo_entry->bouquet, node->data, sizeof(bouquetEntry));
					undo_entry->node = NULL;
				break;
			}
		break; // END OF case VIEW_BOUQUETS

		case VIEW_SETTINGS:
		break; // case VIEW_SETTINGS
	}
	undo_list = g_list_prepend(undo_list, undo_entry);
	MW_SET("UNDO_LIST", undo_list);
	if (lastJob != undoJob) countUndo(+1);
	lastJob = undoJob;
	gtk_widget_set_sensitive(MW_GET("BUT_UNDO"), TRUE);
}

////////////////////////////////////////////////////////////////////////////////
// make an undo step
////////////////////////////////////////////////////////////////////////////////
void but_Undo(gpointer callback_data, guint  callback_action, GtkWidget *widget){
	gint jobNr;
	undo *undo_entry;
	GNode *undo_node;
	GList *undo_list = g_list_first(MW_GET("UNDO_LIST"));
	GtkTreeModel* model;
	GtkTreeIter iter1;
	GtkTreeSelection* selection;
	gint unselect =0;
	
	if (!undo_list) return;
	redoJob++;
	do{
		undo_entry = undo_list->data;
		switch (undo_entry->view){
			case VIEW_CHANNELS:
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
				if (!unselect++) gtk_tree_selection_unselect_all(selection);
				switch(undo_entry->action){

					case UNDO_SWAP:
						pushRedo(VIEW_CHANNELS, UNDO_SWAP, undo_entry->pos, undo_entry->node, TRUE);
						swap_row(VIEW_CHANNELS, (gint)undo_entry->node, undo_entry->pos);
					break;

					case UNDO_MOVE:
						pushRedo(VIEW_CHANNELS, UNDO_MOVE, undo_entry->pos, undo_entry->node, TRUE);
						move_row(VIEW_CHANNELS, (gint)undo_entry->node, undo_entry->pos);
					break;

					case UNDO_COPY:
						// (pos == -1) -> (pos = last entry).
						if (undo_entry->pos <0) undo_entry->pos = g_node_n_children(undo_entry->node)-1;
						if (MW_GET("AKT_BOUQUET") == undo_entry->node) {
							statusbar *status = MW_GET("RIGHT_STATUSBAR");
							gtk_tree_model_iter_nth_child(model, &iter1, NULL, undo_entry->pos);
							gtk_tree_model_get(model, &iter1, RIGHT_CHANNEL_LIST__NODE,  &undo_node, -1);
							gtk_list_store_remove(GTK_LIST_STORE(model), &iter1);
							if (gtk_tree_model_iter_nth_child(model, &iter1, NULL, undo_entry->pos))
       					gtk_tree_selection_select_iter(selection, &iter1);
							status->sumChannels[getServicePic(((channelEntry *)undo_node->data)->serviceType)]--;
							updateStatusbar(status);
						}
						undo_node = g_node_nth_child(undo_entry->node, undo_entry->pos);
						pushRedo(VIEW_CHANNELS, UNDO_DELETE, undo_entry->pos, undo_node, TRUE);
						remove_node(undo_node);
					break;

					case UNDO_DELETE:
					{
						statusbar *status = MW_GET("RIGHT_STATUSBAR");
						GdkPixbuf **serviceType = MW_GET("SERVICE_TYPE_GFX");
						channelEntry *channel = malloc(sizeof(channelEntry));
						memcpy(channel, undo_entry->channel, sizeof(channelEntry));
						undo_node = g_node_insert(MW_GET("AKT_BOUQUET"), undo_entry->pos, g_node_new(channel));
						pushRedo(VIEW_CHANNELS, UNDO_COPY, undo_entry->pos, MW_GET("AKT_BOUQUET"), TRUE);
						gtk_list_store_insert(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter1, undo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter1,
							RIGHT_CHANNEL_LIST__DISEQC , undo_entry->channel->diseqc,
							RIGHT_CHANNEL_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
							RIGHT_CHANNEL_LIST__CHANNEL, txtIn(channel->channelName),
							RIGHT_CHANNEL_LIST__EDITABLE, TRUE,
							RIGHT_CHANNEL_LIST__NODE, undo_node, -1);
						gtk_tree_selection_select_iter(selection, &iter1);
						status->sumChannels[getServicePic(channel->serviceType)]++;
						updateStatusbar(status);
					}
					break;

					case UNDO_RENAME:
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, undo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter1,
							RIGHT_CHANNEL_LIST__CHANNEL, txtIn(undo_entry->channel->channelName), -1);
						gtk_tree_model_get(model, &iter1, RIGHT_CHANNEL_LIST__NODE, &undo_node, -1);
						pushRedo(VIEW_CHANNELS, UNDO_RENAME, undo_entry->pos, undo_node, TRUE);
						strcpy(((channelEntry*)undo_node->data)->channelName,
      				txtIn(undo_entry->channel->channelName));
						free(undo_entry->channel);
						gtk_tree_selection_select_iter(selection, &iter1);
					break;
				}
			break; // END OF case VIEW_CHANNELS

			case VIEW_BOUQUETS:
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
				switch(undo_entry->action){

					case UNDO_SWAP:
						pushRedo(VIEW_BOUQUETS, UNDO_SWAP, undo_entry->pos, undo_entry->node, TRUE);
      			swap_row(VIEW_BOUQUETS, (gint)undo_entry->node, undo_entry->pos);
					break;

					case UNDO_MOVE:
						pushRedo(VIEW_BOUQUETS, UNDO_MOVE, undo_entry->pos, undo_entry->node, TRUE);
						move_row(VIEW_BOUQUETS, (gint)undo_entry->node, undo_entry->pos);
					break;

					case UNDO_COPY:
						undo_node =  g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"),undo_entry->pos);
						pushRedo(VIEW_BOUQUETS, UNDO_DELETE, undo_entry->pos, undo_node, TRUE);
						remove_node(undo_node);
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, undo_entry->pos);
						gtk_list_store_remove(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1);
						if (!g_node_first_child(MW_GET("RIGHT_NODE_ROOT"))
						|| GTK_WIDGET_VISIBLE(MW_GET("BOUQUET_SCROLLER"))) break;
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, undo_entry->pos-1);
						gtk_tree_selection_select_iter(selection, &iter1);
					break;

					case UNDO_DELETE:
					{
						bouquetEntry *bouquet = malloc(sizeof(bouquetEntry));
						memcpy(bouquet, undo_entry->bouquet, sizeof(bouquetEntry));
						undo_node = g_node_insert(MW_GET("RIGHT_NODE_ROOT"),
      				undo_entry->pos, g_node_new(bouquet));
						pushRedo(VIEW_BOUQUETS, UNDO_COPY, undo_entry->pos, NULL, TRUE);
						gtk_list_store_insert(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1, undo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1,
							RIGHT_BOUQUET_LIST__HIDDEN , bouquet->hidden,
							RIGHT_BOUQUET_LIST__LOCKED , bouquet->locked,
							RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquet->bouquetName),
							RIGHT_BOUQUET_LIST__EDITABLE, TRUE,
							RIGHT_BOUQUET_LIST__NODE, undo_node, -1);
						MW_SET("AKT_BOUQUET", undo_node);
						gtk_tree_selection_select_iter(selection, &iter1);
					}
					break;

					case UNDO_RENAME:
					case UNDO_HIDE_LOCK:
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, undo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1,
							RIGHT_BOUQUET_LIST__BOUQUET, txtIn(undo_entry->bouquet->bouquetName),
							RIGHT_BOUQUET_LIST__HIDDEN, undo_entry->bouquet->hidden,
							RIGHT_BOUQUET_LIST__LOCKED, undo_entry->bouquet->locked,
							-1);
						gtk_tree_model_get(model, &iter1, RIGHT_BOUQUET_LIST__NODE, &undo_node, -1);
						pushRedo(VIEW_BOUQUETS, undo_entry->action, undo_entry->pos, undo_node, TRUE);
						memcpy(undo_node->data, undo_entry->bouquet, sizeof(bouquetEntry));
						free(undo_entry->bouquet);
						gtk_tree_selection_select_iter(selection, &iter1);
					break;
			}
			break; // END OF case VIEW_BOUQUETS
			
			case VIEW_SETTINGS:
				switch(undo_entry->action){
					case UNDO_CHANNEL_VIEW:
						but_toggleViewMode((gpointer) BOUQUET_VIEW, TRUE);
						pushRedo(VIEW_SETTINGS,  UNDO_BOUQUET_VIEW,
							g_node_child_position(MW_GET("RIGHT_NODE_ROOT"), MW_GET("AKT_BOUQUET")), NULL, TRUE);
					break;
					case UNDO_BOUQUET_VIEW:
						if (undo_entry->pos <0){ // (spezial) undo from copy all!
      				pushRedo(VIEW_SETTINGS, UNDO_BOUQUET_VIEW, -1, NULL, TRUE);
							MW_SET("AKT_BOUQUET", g_node_first_child(MW_GET("RIGHT_NODE_ROOT")));
						}else{
      				pushRedo(VIEW_SETTINGS, UNDO_CHANNEL_VIEW, 0, NULL, TRUE);
							MW_SET("AKT_BOUQUET", g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"),	undo_entry->pos));
						}
						but_toggleViewMode((gpointer) CHANNEL_VIEW, TRUE);
					break;
				}
			break; // END OF case VIEW_SETTINGS
		}
		jobNr = undo_entry->jobNr;
		undo_list = g_list_remove(undo_list, undo_entry);
		free(undo_entry);
	}while (undo_list && (jobNr == ((undo*)undo_list->data)->jobNr));

	MW_SET("UNDO_LIST", undo_list);
	countUndo(-1);
	if(!MW_GET("AKT_BOUQUET")) gtk_label_set_text(MW_GET("TOGGLE_LABEL"), _("<Bouquets are empty>"));
}

////////////////////////////////////////////////////////////////////////////////
// Only MAX_REDO_STEPS are allowed.
////////////////////////////////////////////////////////////////////////////////
gint countRedo(gint delta){
	static gint steps =0;
	gint jobNr;
	undo *redo_entry;
	GList *redo_list = MW_GET("REDO_LIST");

	if (!delta) steps = 0;
	steps+= delta;
	if (steps > MAX_UNDO_STEPS) { // delete one Redo-step.
		do{
			redo_entry = g_list_last(redo_list)->data;
			jobNr = redo_entry->jobNr;
			if (redo_entry->channel) free(redo_entry->channel);
			if (redo_entry->bouquet) free(redo_entry->bouquet);
			redo_list = g_list_remove(redo_list, redo_entry);
			free(redo_entry);
		}while (jobNr == ((undo *)g_list_last(redo_list)->data)->jobNr);
		steps--;
	}
	MW_SET("REDO_LIST", redo_list);
	if (!steps) gtk_widget_set_sensitive(MW_GET("BUT_REDO"), FALSE);
	return steps;
}

////////////////////////////////////////////////////////////////////////////////
// Clear Redo Buffer.
////////////////////////////////////////////////////////////////////////////////
void clearRedo(){
	undo  *redo_entry;
	GList *redo_list = MW_GET("REDO_LIST");

	while (redo_list){
		redo_entry = redo_list->data;
		if (redo_entry->channel) free(redo_entry->channel);
		if (redo_entry->bouquet) free(redo_entry->bouquet);
		redo_list = g_list_remove(redo_list, redo_entry);
		free(redo_entry);
	}
	countRedo(0);
	MW_SET("REDO_LIST", NULL);
}

////////////////////////////////////////////////////////////////////////////////
// remember an redo step
////////////////////////////////////////////////////////////////////////////////
void pushRedo(gint view, gint action, gint pos, GNode *node, gboolean callFromUndo){
	static gint lastJob = -123456;
	GList *redo_list = MW_GET("REDO_LIST");
	undo *redo_entry = malloc(sizeof(undo));
	redo_entry->pos = pos;
	redo_entry->view = view;
	redo_entry->jobNr = redoJob;
	redo_entry->action = action;
	redo_entry->node = node;
	redo_entry->channel = NULL;
	redo_entry->bouquet = NULL;

	switch (redo_entry->view){
		case VIEW_CHANNELS:
			switch (action){
				case UNDO_DELETE:
				case UNDO_RENAME:
					redo_entry->channel = malloc(sizeof(channelEntry));
					memcpy(redo_entry->channel, node->data, sizeof(channelEntry));
					redo_entry->node = NULL;
				break;
			}
		break; // END OF case VIEW_CHANNELS

		case VIEW_BOUQUETS:
			switch (action){
				case UNDO_RENAME:
				case UNDO_HIDE_LOCK:
				case UNDO_DELETE:
   				redo_entry->bouquet = malloc(sizeof(bouquetEntry));
					memcpy(redo_entry->bouquet, node->data, sizeof(bouquetEntry));
					redo_entry->node = NULL;
     		break;
			}
		break; // END OF case VIEW_BOUQUETS

		case VIEW_SETTINGS:
		break; // case VIEW_SETTINGS
	}
	redo_list = g_list_prepend(redo_list, redo_entry);
	MW_SET("REDO_LIST", redo_list);
	if (lastJob != redoJob) countRedo(+1);
	lastJob = redoJob;
	gtk_widget_set_sensitive(MW_GET("BUT_REDO"), TRUE);
}

////////////////////////////////////////////////////////////////////////////////
// make an redo step
////////////////////////////////////////////////////////////////////////////////
void but_Redo(gpointer callback_data, guint  callback_action, GtkWidget *widget){
	gint jobNr;
	undo *redo_entry;
	GNode *redo_node;
	GList *redo_list = g_list_first(MW_GET("REDO_LIST"));
	GtkTreeModel* model;
	GtkTreeIter iter1;
	GtkTreeSelection* selection;
	gint unselect =0;

	if (!redo_list) return;
	undoJob++;
	do{
		redo_entry = redo_list->data;
		switch (redo_entry->view){
			case VIEW_CHANNELS:
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
				if (!unselect++) gtk_tree_selection_unselect_all(selection);
				switch(redo_entry->action){

					case UNDO_SWAP:
						pushUndo(VIEW_CHANNELS, UNDO_SWAP, redo_entry->pos, redo_entry->node, TRUE);
						swap_row(VIEW_CHANNELS, redo_entry->pos, (gint)redo_entry->node);
					break;

					case UNDO_MOVE:
						pushUndo(VIEW_CHANNELS, UNDO_MOVE, redo_entry->pos, redo_entry->node, TRUE);
						move_row(VIEW_CHANNELS, redo_entry->pos, (gint)redo_entry->node);
					break;

					case UNDO_COPY:
						// (pos == -1) -> (pos = last entry).
						if (redo_entry->pos <0) redo_entry->pos = g_node_n_children(redo_entry->node) -1;
						if (MW_GET("AKT_BOUQUET") == redo_entry->node) {
							statusbar *status = MW_GET("RIGHT_STATUSBAR");
							gtk_tree_model_iter_nth_child(model, &iter1, NULL, redo_entry->pos);
							gtk_tree_model_get(model, &iter1, RIGHT_CHANNEL_LIST__NODE,  &redo_node, -1);
							gtk_list_store_remove(GTK_LIST_STORE(model), &iter1);
							if (gtk_tree_model_iter_nth_child(model, &iter1, NULL, redo_entry->pos))
								gtk_tree_selection_select_iter(selection, &iter1);
							status->sumChannels[getServicePic(((channelEntry *)redo_node->data)->serviceType)]--;
							updateStatusbar(status);
						}
						redo_node = g_node_nth_child(redo_entry->node, redo_entry->pos);
						pushUndo(VIEW_CHANNELS, UNDO_DELETE, redo_entry->pos, redo_node, TRUE);
						remove_node(redo_node);
					break;

					case UNDO_DELETE:
					{
						statusbar *status = MW_GET("RIGHT_STATUSBAR");
						GdkPixbuf **serviceType = MW_GET("SERVICE_TYPE_GFX");
						channelEntry *channel = malloc(sizeof(channelEntry));
						memcpy(channel, redo_entry->channel, sizeof(channelEntry));
						redo_node = g_node_insert(MW_GET("AKT_BOUQUET"), redo_entry->pos, g_node_new(channel));
						pushUndo(VIEW_CHANNELS, UNDO_COPY, redo_entry->pos, MW_GET("AKT_BOUQUET"), TRUE);
						gtk_list_store_insert(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter1, redo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter1,
							RIGHT_CHANNEL_LIST__DISEQC , redo_entry->channel->diseqc,
							RIGHT_CHANNEL_LIST__SERVICE, serviceType[getServicePic(channel->serviceType)],
							RIGHT_CHANNEL_LIST__CHANNEL, txtIn(channel->channelName),
							RIGHT_CHANNEL_LIST__EDITABLE, TRUE,
							RIGHT_CHANNEL_LIST__NODE, redo_node, -1);
						gtk_tree_selection_select_iter(selection, &iter1);
						status->sumChannels[getServicePic(channel->serviceType)]++;
						updateStatusbar(status);
					}
					break;

					case UNDO_RENAME:
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, redo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_CHANNEL_STORE"), &iter1,
							RIGHT_CHANNEL_LIST__CHANNEL, txtIn(redo_entry->channel->channelName), -1);
						gtk_tree_model_get(model, &iter1, RIGHT_CHANNEL_LIST__NODE, &redo_node, -1);
						pushUndo(VIEW_CHANNELS, UNDO_RENAME, redo_entry->pos, redo_node, TRUE);
						strcpy(((channelEntry*)redo_node->data)->channelName,
      				txtIn(redo_entry->channel->channelName));
						free(redo_entry->channel);
						gtk_tree_selection_select_iter(selection, &iter1);
					break;
				}
			break; // END OF case VIEW_CHANNELS

			case VIEW_BOUQUETS:
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(MW_GET("RIGHT_BOUQUET_TREEVIEW")));
				switch(redo_entry->action){

					case UNDO_SWAP:
						pushUndo(VIEW_BOUQUETS, UNDO_SWAP, redo_entry->pos, redo_entry->node, TRUE);
						swap_row(VIEW_BOUQUETS, redo_entry->pos, (gint)redo_entry->node);
					break;

					case UNDO_MOVE:
						pushUndo(VIEW_BOUQUETS, UNDO_MOVE, redo_entry->pos, redo_entry->node, TRUE);
						move_row(VIEW_BOUQUETS, redo_entry->pos, (gint)redo_entry->node);
					break;

					case UNDO_COPY:
						redo_node =  g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"),redo_entry->pos);
						pushUndo(VIEW_BOUQUETS, UNDO_DELETE, redo_entry->pos, redo_node, TRUE);
						remove_node(redo_node);
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, redo_entry->pos);
						gtk_list_store_remove(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1);
						if (!g_node_first_child(MW_GET("RIGHT_NODE_ROOT"))
						|| GTK_WIDGET_VISIBLE(MW_GET("BOUQUET_SCROLLER"))) break;
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, redo_entry->pos-1);
						gtk_tree_selection_select_iter(selection, &iter1);
					break;

					case UNDO_DELETE:
					{
						bouquetEntry *bouquet = malloc(sizeof(bouquetEntry));
						memcpy(bouquet, redo_entry->bouquet, sizeof(bouquetEntry));
						redo_node = g_node_insert(MW_GET("RIGHT_NODE_ROOT"), redo_entry->pos,
      				g_node_new(bouquet));
						pushUndo(VIEW_BOUQUETS, UNDO_COPY, redo_entry->pos, NULL, TRUE);
						gtk_list_store_insert(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1, redo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1,
							RIGHT_BOUQUET_LIST__HIDDEN , bouquet->hidden,
							RIGHT_BOUQUET_LIST__LOCKED , bouquet->locked,
							RIGHT_BOUQUET_LIST__BOUQUET, txtIn(bouquet->bouquetName),
							RIGHT_BOUQUET_LIST__EDITABLE, TRUE,
							RIGHT_BOUQUET_LIST__NODE, redo_node, -1);
						MW_SET("AKT_BOUQUET", redo_node);
     				gtk_tree_selection_select_iter(selection, &iter1);
					}
					break;

					case UNDO_RENAME:
					case UNDO_HIDE_LOCK:
						gtk_tree_model_iter_nth_child(model, &iter1, NULL, redo_entry->pos);
						gtk_list_store_set(MW_GET("RIGHT_LIST_BOUQUET_STORE"), &iter1,
							RIGHT_BOUQUET_LIST__BOUQUET, txtIn(redo_entry->bouquet->bouquetName),
							RIGHT_BOUQUET_LIST__HIDDEN, redo_entry->bouquet->hidden,
							RIGHT_BOUQUET_LIST__LOCKED, redo_entry->bouquet->locked,
							-1);
						gtk_tree_model_get(model, &iter1, RIGHT_BOUQUET_LIST__NODE, &redo_node, -1);
						pushUndo(VIEW_BOUQUETS, redo_entry->action, redo_entry->pos, redo_node, TRUE);
						memcpy(redo_node->data, redo_entry->bouquet, sizeof(bouquetEntry));
						free(redo_entry->bouquet);
						gtk_tree_selection_select_iter(selection, &iter1);
					break;
			}
			break; // END OF case VIEW_BOUQUETS

			case VIEW_SETTINGS:
				switch(redo_entry->action){
					case UNDO_CHANNEL_VIEW:
						but_toggleViewMode((gpointer) BOUQUET_VIEW, TRUE);
						pushUndo(VIEW_SETTINGS, UNDO_BOUQUET_VIEW,
							g_node_child_position(MW_GET("RIGHT_NODE_ROOT"), MW_GET("AKT_BOUQUET")), NULL, TRUE);
					break;
					case UNDO_BOUQUET_VIEW:
						if (redo_entry->pos <0){ // (spezial) undo from copy all!
      				pushUndo(VIEW_SETTINGS, UNDO_BOUQUET_VIEW, -1, NULL, TRUE);
							MW_SET("AKT_BOUQUET", g_node_first_child(MW_GET("RIGHT_NODE_ROOT")));
						}else{
      				pushUndo(VIEW_SETTINGS, UNDO_CHANNEL_VIEW, 0, NULL, TRUE);
							MW_SET("AKT_BOUQUET", g_node_nth_child(MW_GET("RIGHT_NODE_ROOT"),	redo_entry->pos));
						}
						but_toggleViewMode((gpointer) CHANNEL_VIEW, TRUE);
					break;
				}
			break; // END OF case VIEW_SETTINGS
		}
		jobNr = redo_entry->jobNr;
		redo_list = g_list_remove(redo_list, redo_entry);
		free(redo_entry);
	}while (redo_list && (jobNr == ((undo*)redo_list->data)->jobNr));

	MW_SET("REDO_LIST", redo_list);
	countRedo(-1);
	if(!MW_GET("AKT_BOUQUET")) gtk_label_set_text(MW_GET("TOGGLE_LABEL"), _("<Bouquets are empty>"));
}

////////////////////////////////////////////////////////////////////////////////
// center window
////////////////////////////////////////////////////////////////////////////////
void center_window(GtkWindow *win) {
/*
	static WDesktopInfo d;
	gint w, h, x, y;

	gdk_window_get_geometry(gdk_get_default_root_window(),NULL,NULL,&d.width,&d.height,&d.depth);
	gtk_window_get_size(win,&w,&h);
	x = (d.width / 2) - (w /2);
	y = (d.height /2) - (h /2);
	gtk_window_move(win,x,y);
//	gtk_window_set_position(win, GTK_WIN_POS_CENTER);
*/
}

////////////////////////////////////////////////////////////////////////////////
// Find string (not case sensitive).
////////////////////////////////////////////////////////////////////////////////
gint strFind(gchar *text1, gchar *text2){
	gchar *ptr=text2;
	while (*text1){
		if ((*text1++ |32) == (*text2++ |32)){
			if (*text2==0) return 1;
		}
		else text2=ptr;
	}
	return 0; // not found.
}

////////////////////////////////////////////////////////////////////////////////
// Save the actual bouquet.
////////////////////////////////////////////////////////////////////////////////
void but_saveAktBouquet(gpointer callback_data, guint  callback_action, GtkWidget *widget){
	if (!MW_GET("AKT_BOUQUET")) return;
	saveLCars();
	saveNeutrino(0);
}

////////////////////////////////////////////////////////////////////////////////
// Save all bouquets.
////////////////////////////////////////////////////////////////////////////////
void but_saveAllBouquet(gpointer callback_data, guint  callback_action, GtkWidget *widget){
	if (!MW_GET("AKT_BOUQUET")) return;
	saveNeutrino(-1);
	saveEnigma();
}

////////////////////////////////////////////////////////////////////////////////
// Append current path to filename.
////////////////////////////////////////////////////////////////////////////////
gchar *get_path (gchar *filename){
	static gchar file_path[BUFFER_SIZE+1];

	getcwd(file_path, BUFFER_SIZE);
	#ifndef WIN32
	strcat(file_path,"/");
	#else
	strcat(file_path,"\\");
	#endif
	strcat(file_path, filename);
	return file_path;
}

////////////////////////////////////////////////////////////////////////////////
// compare strings (without case, without umlaute)
////////////////////////////////////////////////////////////////////////////////
gint cmpStr(guchar *txt1, guchar *txt2){
	static const guchar charTab[]={
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	 96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126, 32,
	 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 83, 32, 79, 32, 90, 32,
	 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 83, 32, 79, 32, 90, 89,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	 65, 65, 65, 65, 65, 65, 65, 67, 69, 69, 69, 69, 73, 73, 73, 73,
	 68, 78, 79, 79, 79, 79, 79,215, 79, 85, 85, 85, 85, 89, 80, 83,
	 64, 68, 69, 69, 69, 32, 73, 73, 73, 32, 32, 32, 32, 32, 73, 32,
	 68, 78, 79, 79, 79, 79, 79,247, 79, 85, 85, 85, 85, 89, 80, 89};

	while(*txt1 && *txt2)	{
		if (charTab[*txt1]!=charTab[*txt2]) break;
		txt1++;
		txt2++;
	}
	return (charTab[*txt1]-charTab[*txt2]);
}

////////////////////////////////////////////////////////////////////////////////
// UTF-8 convert
////////////////////////////////////////////////////////////////////////////////
gchar *txtOut(gchar *text){
	//return g_locale_from_utf8(text, -1, NULL, NULL, NULL);
	return g_convert(text, -1, "Latin1", "UTF-8", NULL, NULL, NULL);
}

gchar *txtIn(gchar *text){
	return g_convert(text, -1, "UTF-8", "Latin1", NULL, NULL, NULL);
//	return g_convert(text, -1, "UTF-8", "ISO-8859-15", NULL, NULL, NULL); 
}

////////////////////////////////////////////////////////////////////////////////
// Get pic-number form serviceType.
////////////////////////////////////////////////////////////////////////////////
gint getServicePic(gint serviceType){
	static gint pics[256]={
		DATA, TV,RADIO,DATA,NVOD,NVOD,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,
		DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA,DATA};
	return pics[(unsigned char)serviceType];
}

////////////////////////////////////////////////////////////////////////////////
// write the config-file.
////////////////////////////////////////////////////////////////////////////////
gchar* cfg_write(gchar *section, gchar *key, gchar *value){
	static gchar val[CFG_ENTRY_MAXLEN+1];
	struct s_entry{
		gchar section[CFG_ENTRY_MAXLEN+1];
		gchar key    [CFG_ENTRY_MAXLEN+1];
		gchar value  [CFG_ENTRY_MAXLEN+1];
		} *entry;
	gchar sect[CFG_ENTRY_MAXLEN+1];
	gchar buffer[BUFFER_SIZE+1];

	FILE *stream;
	gint pos;
	GList *cfg_list = NULL;
	GList *cfg_list_p;
	gint found;

	//======== copy config-file into cfg_list.
	if ((stream = fopen(get_path(CFG_FILENAME), "rb")))
	{
		gchar *txtPos;
		while (fgets(buffer,BUFFER_SIZE,stream)){
			txtPos=buffer;
			while(*txtPos && *txtPos <= ' ' ) txtPos++; // skip Whitespace.
			if (*txtPos == '#' || *txtPos == ';' || *txtPos==0 ) continue; // comment
			if (*txtPos == '[')	{ // New Section found.
				pos=0;
				while ((*(++txtPos) != ']') && pos < CFG_ENTRY_MAXLEN) sect[pos++]=*txtPos;
				sect[pos]=0;
				continue;
			}
			entry= malloc(sizeof(struct s_entry));
			strncpy(entry->section ,sect,  CFG_ENTRY_MAXLEN);
			pos=0;
			while ( *txtPos > '\n' && *txtPos!= '=' && pos < CFG_ENTRY_MAXLEN)
   			entry->key[pos++] = *txtPos++;
			entry->key[pos]=0;
			pos=0;
			if (*txtPos++ == '=')
   			while (*txtPos > '\n' && pos < CFG_ENTRY_MAXLEN) entry->value[pos++] = *txtPos++;
			entry->value[pos]=0;
			cfg_list = g_list_append (cfg_list, (gpointer) entry);
		}
		fclose(stream);
	}
	//======== update value
	pos=0;
	found=0;
	cfg_list_p = g_list_first(cfg_list);
	while (cfg_list_p){
		entry = (struct s_entry *)cfg_list_p->data;
		if (!strcmp(entry->section, section)){
			found=1;
			if(!strcmp(entry->key, key)){
				found=2;
				strncpy(entry->value , value, CFG_ENTRY_MAXLEN);
				break;
			}
		}else{
			if (found==1) break;
			pos++;
		}
		cfg_list_p = g_list_next(cfg_list_p);
	}
	//======== neues Element einfügen.	
	if (found!=2){
		cfg_list_p = g_list_first(cfg_list);
		entry= malloc(sizeof(struct s_entry));
		strncpy(entry->section, section, CFG_ENTRY_MAXLEN);
		strncpy(entry->key,     key,     CFG_ENTRY_MAXLEN);
		strncpy(entry->value,   value,   CFG_ENTRY_MAXLEN);	
		cfg_list = g_list_insert (cfg_list, (gpointer) entry,pos);
	}
	//========= cfg-datei speichern.
	if ((stream = fopen(get_path(CFG_FILENAME), "wb"))){
		sect[0]=0;
		cfg_list_p = g_list_first(cfg_list);
		while (cfg_list_p){
			entry = (struct s_entry *)cfg_list_p->data;
			if (strcmp(sect, entry->section)){
				fputs("\n",stream);	
				strcpy(sect, entry->section);
				fputs("[",stream);
				fputs(entry->section,stream);
				fputs("]\n",stream);
			}
			fputs(entry->key,stream);
			fputs("=",stream);
			fputs(entry->value,stream);
			fputs("\n",stream);
			cfg_list_p = g_list_next(cfg_list_p);
		}
		fclose(stream);
	}
	//======== cfg_list komlett löschen.
	cfg_list_p = g_list_first(cfg_list);
	while (cfg_list_p){
		entry = (struct s_entry *)cfg_list_p->data;
		free(entry);
		cfg_list_p = g_list_next(cfg_list_p);
	}
	g_list_free(cfg_list);

	strncpy(val, value, CFG_ENTRY_MAXLEN);
	return val;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
gchar *cfg_read (gchar *section, gchar *key, gchar *value){
	static gchar sect[CFG_ENTRY_MAXLEN+1];
	gchar buffer[BUFFER_SIZE+1];
	FILE *stream;

	if ((stream = fopen(get_path(CFG_FILENAME), "rb"))){
		gboolean found=FALSE;
		gchar *buf = buffer;
		gint pos=0;

		sprintf(sect,"[%s]",section);
		while (fgets(buffer,BUFFER_SIZE,stream)) if (strstr(buffer, sect)) break;
		while (!found && fgets(buffer,BUFFER_SIZE,stream)) if (strstr(buffer, key)) found = TRUE;
		fclose(stream);
		if (!found) return cfg_write(section, key, value);
		while (*buf++ !='=');
		while (*buf >= ' ') sect[pos++] = *buf++;
		if (!pos) return cfg_write(section, key, value);
		sect[pos]=0;
		return sect;
	}
	return cfg_write(section, key, value);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void but_writeConfig(){
	gboolean show;
	gchar value[10];
	gint width, height, posx, posy;

	gtk_window_get_size(MainWindow, &width, &height);
	sprintf(value,"%d",width);  cfg_write("geometry", "window width", value);
	sprintf(value,"%d",height); cfg_write("geometry", "window height", value);
	gtk_window_get_position(MainWindow, &posx, &posy);
	#ifdef WIN32
	posy = 0;
	#endif
	sprintf(value,"%d",posx); cfg_write("geometry", "window posx", value);
	sprintf(value,"%d",posy); cfg_write("geometry", "window posy", value);

	show = gtk_toggle_button_get_active(MW_GET("OPT_SHOW_HEADER"));
	sprintf(value,"%d", show);  cfg_write("listview", "show header", value);
	show = gtk_toggle_button_get_active(MW_GET("OPT_SHOW_DISEQC"));
	sprintf(value,"%d", show);  cfg_write("listview", "show diseqc", value);
	show = gtk_toggle_button_get_active(MW_GET("OPT_SHOW_NUMBERS"));
	sprintf(value,"%d", show);  cfg_write("listview", "show numbers", value);
	show = gtk_toggle_button_get_active(MW_GET("OPT_READ_DATA"));
	sprintf(value,"%d", show);  cfg_write("listview", "show data services", value);
	show = gtk_toggle_button_get_active(MW_GET("OPT_READ_SHORT"));
	sprintf(value,"%d", show);  cfg_write("listview", "show short channelnames", value);
	show = gtk_toggle_button_get_active(MW_GET("OPT_READ_WHITESPACE"));
	sprintf(value,"%d", show);  cfg_write("listview", "show fair channelnames", value);

	show = gtk_toolbar_get_style(GTK_TOOLBAR(MW_GET("LEFT_TOOLBAR")));
	sprintf(value, "%d", show); cfg_write("toolbar", "right toolbar style", value);
	show = gtk_toolbar_get_style(GTK_TOOLBAR(MW_GET("MID_TOOLBAR")));
	sprintf(value, "%d", show); cfg_write("toolbar", "middle toolbar style", value);
	show = gtk_toolbar_get_style(GTK_TOOLBAR(MW_GET("RIGHT_TOOLBAR")));
	sprintf(value, "%d", show); cfg_write("toolbar", "left toolbar style", value);

	cfg_write("network", "ip",(gchar*)gtk_entry_get_text(MW_GET("OPT_NET_IP")));
	cfg_write("network", "path",(gchar*)gtk_entry_get_text(MW_GET("OPT_NET_PATH")));
	cfg_write("network", "user",(gchar*)gtk_entry_get_text(MW_GET("OPT_NET_USER")));
	cfg_write("network", "password",(gchar*)gtk_entry_get_text(MW_GET("OPT_NET_PASS")));
	cfg_write("network", "serial port", (gchar*)gtk_entry_get_text(MW_GET("OPT_SERIAL_PORT")));
}

////////////////////////////////////////////////////////////////////////////////
// save the actual Bouquet as RTF-file.
////////////////////////////////////////////////////////////////////////////////
#define rowBorder(side) "\\clbrdr"#side"\\brdrs\\brdrw5\n"
#define rowPrint(align) "\\pard\\plain\\intbl\\rtlch\\ltrch\\fs12\\q"#align"{\\ltrch\\loch "
#define rowWidth(x)     "\\cellx"#x"\n"
#define rowOpen         "\\trowd\\trql\n"
#define rowNext         "}\\cell\n"
#define rowClose        "}\\cell\\row\\pard\n\n"

void writeRTF(GNode *node_bouquet, gint serviceType, FILE *stream){
	GNode *node_channel = g_node_first_child(node_bouquet);
	gchar txtBuffer[8];
	gint pos = 1;
	gint service;
	fputs(rowOpen rowBorder(t) rowBorder(l) rowBorder(b) rowBorder(r) rowWidth(2000), stream);
	fputs(rowPrint(c), stream);
	fputs(((bouquetEntry*)node_bouquet->data)->bouquetName , stream);
	if (serviceType == TV)    fputs(" (TV)",stream);
	if (serviceType == DATA)  fputs(" (Data)",stream);
	if (serviceType == RADIO) fputs(" (Radio)",stream);
	fputs(rowClose, stream);

	while (node_channel){
		service = getServicePic(((channelEntry*)node_channel->data)->serviceType);
		if ((serviceType == TV  && ( service == TV || service == NVOD))
			|| (serviceType == RADIO && service == RADIO)
			|| (serviceType == DATA  && service == DATA))
		{
			sprintf(txtBuffer, "%.3d", pos++);
			fputs(rowOpen	rowBorder(t) rowBorder(l) rowBorder(b) rowBorder(r) rowWidth(350), stream);
			fputs(rowBorder(t) rowBorder(l) rowBorder(b) rowBorder(r) rowWidth(2000), stream);
			fputs(rowPrint(r) , stream);
			fputs(txtBuffer, stream);
			fputs(rowNext, stream);
			fputs(rowPrint(l), stream);
			fputs(((channelEntry*)node_channel->data)->channelName,	stream);
	  	fputs(rowClose, stream);
		}
		node_channel = node_channel->next;
	}
}

void but_saveAktAsRTF() {
	FILE *stream;
	GNode *node_bouquet = MW_GET("AKT_BOUQUET");
	gchar txtBuffer[200];

	if (!node_bouquet || !g_node_first_child(node_bouquet)) return;
	if (!(stream = fopen(get_path(BOUQUET_TO_RTF), "wb"))){
		GtkWidget* dialog;
		sprintf(txtBuffer,_("File '%s' could not be opened."),BOUQUET_TO_RTF);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	fputs("{\\rtf1\\ansi\\deff0\\viewscale150",stream);
//	fputs("\\paperw11906\\paperh16838\\margl850\\margr850\\margt850\\margb850\\cols3\n\n",stream);
	fputs("\\paperw11906\\paperh16838\\margl850\\margr850\\margt850\\margb850\n\n",stream);
//	fputs("\\colsx700\\colno1\\colw3000",stream);
//	fputs("\\colsr700\\colno2\\colw3000",stream);
//	fputs("\\colsr700\\colno3\\colw3000\\sectdefaultcl\n \n",stream);

	writeRTF(node_bouquet, TV, stream);
//	fputs("{\\column\\par}", stream);
//	fputs("\\column\\par", stream);

	fputs("\\par\\par", stream);
  writeRTF(node_bouquet, RADIO, stream);
//	fputs("{\\column\\par}", stream);
	fputs("\\par\\par", stream);
	writeRTF(node_bouquet, DATA , stream);
	fputs("}\n",stream);
	fclose(stream);
}


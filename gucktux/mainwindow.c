/***************************************************************************
                          mainwindow.c  -  description
                             -------------------
    begin                : Sam Sep 21 2002
    copyright            : (C) 2002 by Abadon
    email                : 3999@freenet.de
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 **************************************************************************/

#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gsatedit.h"
#include "guis.h"
#include "dialog.h"
#include "tools.h"
#include "listen.h"
#include "ftp.h"

#include "pics/sat.xpm"
#include "pics/copy.xpm"
#include "pics/logo.xpm"
#include "pics/telnet.xpm"
#include "pics/serial.xpm"
#include "pics/upload.xpm"
#include "pics/download.xpm"
#include "pics/hidden1.xpm"
#include "pics/locked1.xpm"
#include "pics/service.xpm"
#include "pics/insert.xpm"

GtkWidget* StockIcon(const gchar* id) {
	GdkPixbuf* icon;
	GtkWidget* image;
	icon  = gtk_widget_render_icon(GTK_WIDGET(MainWindow), id, GTK_ICON_SIZE_LARGE_TOOLBAR,"do_it");
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	return image;
}

extern const GtkTargetEntry target[];
extern const guint targets;

GtkWidget *create_button_panel(){
	GtkWidget* vbox, *button;
	GtkWidget *toolbar;
	GdkPixbuf *icon;
	GtkWidget *image;

	toolbar = gtk_toolbar_new ();
	vbox = gtk_vbox_new(FALSE,0);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),GTK_ORIENTATION_VERTICAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	MW_SET("MID_TOOLBAR",toolbar);
	gtk_box_pack_start(GTK_BOX(vbox),toolbar,TRUE,FALSE,0);

	button = gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		("Undo"), _("Undo for right Listview"),
		NULL, StockIcon(GTK_STOCK_UNDO),
		G_CALLBACK (but_Undo), MainWindow, -1);
	MW_SET("BUT_UNDO",button);
	gtk_widget_set_sensitive(button, FALSE);

	button = gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		N_("Redo"),_("Redo for right listview"),
		NULL, StockIcon(GTK_STOCK_REDO),
		G_CALLBACK (but_Redo), MainWindow, -1);
	MW_SET("BUT_REDO",button);
	gtk_widget_set_sensitive(button,FALSE);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&copy_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	button = gtk_toolbar_insert_item (GTK_TOOLBAR (toolbar),
		_("Copy"), _("copy selected channels"),
		NULL, image,
		G_CALLBACK (but_copy_channels), (gpointer)0, -1);
	MW_SET("BUT_COPY",button);

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&insert_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	button = gtk_toolbar_insert_item (GTK_TOOLBAR (toolbar),
		_("Insert"), "insert selected channels",
		NULL, image,
		G_CALLBACK (but_copy_channels), (gpointer)1, -1);
	MW_SET("BUT_INSERT",button);

	button = gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("Copy all"), _("Copy all Channels"),
		NULL, StockIcon(GTK_STOCK_GO_FORWARD),
		G_CALLBACK (but_copyAll), MainWindow, -1);
	MW_SET("BUT_COPYALL",button);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	button = gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("delete"), _("delete channels"),
		NULL, StockIcon(GTK_STOCK_DELETE),
		G_CALLBACK (but_del_entries), MainWindow, -1);

	gtk_drag_dest_set(GTK_WIDGET(button),GTK_DEST_DEFAULT_ALL, target,targets,
 		GDK_ACTION_COPY|GDK_ACTION_MOVE);
	g_signal_connect(G_OBJECT(button),"drag_data_received", 
		G_CALLBACK(tree_drag_data_received),(gpointer)8);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("clear"), _("delete all Channels"),
		NULL, StockIcon(GTK_STOCK_NEW),
		G_CALLBACK (clear_right_listview), MainWindow, -1);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("Up"), _("Move Up Channels"),
		NULL, StockIcon(GTK_STOCK_GO_UP),
		G_CALLBACK (but_move), (gpointer)0, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("Down"), _("Move Down Channels"),
		NULL, StockIcon(GTK_STOCK_GO_DOWN),
		G_CALLBACK (but_move), (gpointer)1, -1);

	return vbox;
}

GtkWidget *create_option_panel(){
	GtkWidget *vbox, *hbox, *tmpBox;
	GtkWidget *frame, *tempwid;
	GtkWidget *toolbar;
	GtkWidget *button;
	GtkWidget *image;
	GdkPixbuf *icon;
	GtkTooltips *button_bar_tips;
	GList *serialPorts;
	button_bar_tips = gtk_tooltips_new ();
	vbox = gtk_vbox_new(FALSE,0);

	//============ Read Options
	frame = gtk_frame_new(_("Read options"));
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,5);
	tmpBox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),tmpBox);

	button = gtk_check_button_new_with_label(_("short names"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("short channel names"), "");
	gtk_box_pack_start(GTK_BOX(tmpBox), button, FALSE, FALSE, 0);
	MW_SET("OPT_READ_SHORT",button);

	button = gtk_check_button_new_with_label(_("fair names"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("no whitespaces allowed"), "");
	gtk_box_pack_start(GTK_BOX(tmpBox), button, FALSE, FALSE, 0);
	MW_SET("OPT_READ_WHITESPACE",button);

	button = gtk_check_button_new_with_label(_("read data channels"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("read data channels too"), "");
	gtk_box_pack_start(GTK_BOX(tmpBox), button, FALSE, FALSE, 0);
	MW_SET("OPT_READ_DATA",button);

	//============ Network Options
	frame = gtk_frame_new(_("Network"));
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,5);
	tmpBox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),tmpBox);

	gtk_container_add(GTK_CONTAINER(tmpBox), hbox = gtk_hbox_new(FALSE, 0));
	tempwid= gtk_label_new(_("Path:"));
	gtk_misc_set_alignment(GTK_MISC(tempwid), 0, 0.5);
	gtk_widget_set_usize(tempwid, 40, 24);
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, FALSE, FALSE, 3);

	tempwid= gtk_entry_new_with_max_length(60);
	gtk_widget_set_usize(tempwid, 90, 0);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), tempwid,
		_("Der Hauptfad der Senderlisten auf der dbox."), "");
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, TRUE, TRUE, 0);
	MW_SET("OPT_NET_PATH",tempwid);

	gtk_container_add(GTK_CONTAINER(tmpBox), hbox = gtk_hbox_new(FALSE, 0));
	tempwid= gtk_label_new("IP:");
	gtk_misc_set_alignment(GTK_MISC(tempwid), 0, 0.5);
	gtk_widget_set_usize(tempwid, 40, 24);
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, FALSE, FALSE, 3);

	tempwid= gtk_entry_new_with_max_length(20);
	gtk_widget_set_usize(tempwid, 90, 0);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), tempwid,
		_("The IP of the dbox."), "");
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, TRUE, TRUE, 0);
	MW_SET("OPT_NET_IP",tempwid);

	gtk_container_add(GTK_CONTAINER(tmpBox), hbox = gtk_hbox_new(FALSE, 0));
	tempwid= gtk_label_new("ID:");
	gtk_misc_set_alignment(GTK_MISC(tempwid), 0, 0.5);
	gtk_widget_set_usize(tempwid, 40, 24);
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, FALSE, FALSE, 3);

	tempwid= gtk_entry_new_with_max_length(15);
	gtk_widget_set_usize(tempwid, 10, 0);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), tempwid, _("Your username."), "");
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, TRUE, TRUE, 0);
	MW_SET("OPT_NET_USER",tempwid);

	tempwid= gtk_entry_new_with_max_length(15);
	gtk_widget_set_usize(tempwid, 30, 0);
	gtk_entry_set_visibility(GTK_ENTRY(tempwid), FALSE);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), tempwid, _("Your password."), "");
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, TRUE, TRUE, 0);
	MW_SET("OPT_NET_PASS",tempwid);



	gtk_container_add(GTK_CONTAINER(tmpBox), hbox = gtk_hbox_new(FALSE, 0));
	tempwid= gtk_label_new(_("Serial:"));
	gtk_misc_set_alignment(GTK_MISC(tempwid), 0, 0.5);
	gtk_widget_set_usize(tempwid, 40, 24);
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, FALSE, FALSE, 3);

  tempwid = gtk_combo_new();
	gtk_widget_set_usize(tempwid, 30, 0);
	gtk_widget_set_sensitive(GTK_COMBO(tempwid)->entry, FALSE);
	serialPorts = MW_GET("GLIST_PORTS");
	#ifdef WIN32
  serialPorts = g_list_append(serialPorts, "COM1:");
  serialPorts = g_list_append(serialPorts, "COM2:");
  serialPorts = g_list_append(serialPorts, "COM3:");
  serialPorts = g_list_append(serialPorts, "COM4:");
  #else
  serialPorts = g_list_append(serialPorts, "/dev/ttyS0");
  serialPorts = g_list_append(serialPorts, "/dev/ttyS1");
  serialPorts = g_list_append(serialPorts, "/dev/ttyS2");
  serialPorts = g_list_append(serialPorts, "/dev/ttyS3");
  #endif
  gtk_combo_set_popdown_strings(GTK_COMBO(tempwid), serialPorts);
	gtk_tooltips_set_tip (GTK_TOOLTIPS(button_bar_tips), tempwid,
 		_("The serial port for dbox connection."), "");
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, TRUE, TRUE, 0);
	MW_SET("OPT_SERIAL_PORT", GTK_ENTRY(GTK_COMBO(tempwid)->entry));

  gtk_container_add(GTK_CONTAINER(tmpBox), hbox = gtk_hbox_new(FALSE, 0));
	tempwid= gtk_label_new("Status:");
	gtk_misc_set_alignment(GTK_MISC(tempwid), 0, 0.5);
	gtk_widget_set_usize(tempwid, 40, 24);
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, FALSE, FALSE, 3);

	tempwid= gtk_entry_new_with_max_length(25);
	gtk_widget_set_usize(tempwid,  90, 0);
	gtk_entry_set_text(GTK_ENTRY(tempwid),_("not connected"));
	gtk_widget_set_sensitive(tempwid,FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), tempwid, TRUE, TRUE, 0);
	MW_SET("OPT_NET_STATUS",tempwid);


	toolbar = gtk_toolbar_new ();
//	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),GTK_ORIENTATION_VERTICAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_ICONS);
	MW_SET("NET_TOOLBAR",toolbar);
	gtk_box_pack_start(GTK_BOX(tmpBox),toolbar,TRUE,FALSE,0);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&download_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Senderlisten von dbox holen"),
		NULL, image,
		G_CALLBACK (but_ftp_download), MainWindow, -1);

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&upload_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Senderlisten zur dbox schicken"),
		NULL, image,
		G_CALLBACK (but_ftp_upload), MainWindow, -1);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&telnet_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Telnet-Connection"),
		NULL, image,
		G_CALLBACK (but_telnet_dialog), MainWindow, -1);

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&serial_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Terminal"),
		NULL, image,
		G_CALLBACK (but_serial_dialog), MainWindow, -1);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("show Logfile"),
		NULL, StockIcon(GTK_STOCK_JUSTIFY_LEFT),
		G_CALLBACK(but_ftpLog), MainWindow, -1);

	// zapping toolbar.
	gtk_container_add(GTK_CONTAINER(tmpBox),hbox=gtk_hbox_new(FALSE, 0));
	button = gtk_button_new_with_label(_("reload Bouq."));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("Make upload & refresh Bouquets (Neutrino only)"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
 	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(prepareZap), NULL);

	button = gtk_button_new_with_label(_("zapping"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("Zapping through selected channels (Neutrino only)"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
 	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(zapToChannel), NULL);

	//============ Look & Feel Options
	frame = gtk_frame_new(_("Look & Feel"));
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,5);
	tmpBox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),tmpBox);
	gtk_container_add(GTK_CONTAINER(tmpBox),hbox=gtk_hbox_new(FALSE,0));

	button = gtk_check_button_new_with_label(_("Header"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("show Headers of list-windows"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
 	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_showHeader), (gpointer) button);
	MW_SET("OPT_SHOW_HEADER",button);

	button = gtk_check_button_new_with_label(_("Diseqc"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("show diseqc column"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_showItems), (gpointer) button);
	MW_SET("OPT_SHOW_DISEQC",button);

	button = gtk_check_button_new_with_label(_("Nr 's"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("show channel numbers in act. Bouquet"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_showCount), (gpointer) button);
	MW_SET("OPT_SHOW_NUMBERS",button);

	gtk_container_add(GTK_CONTAINER(tmpBox),hbox=gtk_hbox_new(FALSE,0));
	button = gtk_button_new_with_label(_("toolbar1"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("style of left toolbar"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
 	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_showIconText), (gpointer)0);

	button = gtk_button_new_with_label(_("toolbar2"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("style of mid toolbar"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
 	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_showIconText), (gpointer)1);

	button = gtk_button_new_with_label(_("toolbar3"));
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("style of right toolbar"), "");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
 	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_showIconText), (gpointer)2);

	//============ Logo
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&logo_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);
	//============ Toolbar
	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_ICONS);
	MW_SET("OPTIONS_TOOLBAR",toolbar);
	gtk_box_pack_start(GTK_BOX(vbox),toolbar,FALSE,FALSE,0);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Debug (Abadon only)"),
		NULL, StockIcon(GTK_STOCK_PREFERENCES),
		G_CALLBACK(but_debug), MainWindow, -1);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Info"),
		NULL, StockIcon(GTK_STOCK_DIALOG_QUESTION),
		G_CALLBACK(but_about), MainWindow, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Help"),
		NULL, StockIcon(GTK_STOCK_HELP),
		G_CALLBACK(but_help), MainWindow, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Key Layout"),
		NULL, StockIcon(GTK_STOCK_BOLD),
		G_CALLBACK(but_keyLayout), MainWindow, -1);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		NULL, _("Save settings"),
		NULL, StockIcon(GTK_STOCK_SAVE),
		G_CALLBACK(but_writeConfig), MainWindow, -1);

	return vbox;
}


GtkWidget *create_left_list_panel(){
	statusbar *status;
	GtkCellRenderer* renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GtkWidget *vbox;
	GtkWidget *entry, *hbox;
	GtkWidget *toolbar;
	GtkWidget *scroller, *treeview;
	GtkWidget *image;
	GdkPixbuf *icon;
//	GtkTooltips *statusbar_tips = gtk_tooltips_new ();

	vbox = gtk_vbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox = gtk_hbox_new(FALSE,0), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), StockIcon(GTK_STOCK_FIND), FALSE, FALSE, 0);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
//	g_signal_connect(G_OBJECT(entry), "focus-in-event", GTK_SIGNAL_FUNC(entry_focus), NULL);
//	g_signal_connect(G_OBJECT(entry), "key_press_event",GTK_SIGNAL_FUNC(entry_keypressed), NULL);	
	g_signal_connect_after(G_OBJECT(entry), "changed",  GTK_SIGNAL_FUNC(entry_doSearch), NULL);
	MW_SET("SEARCH_CHANNEL_ENTRY", entry);

	scroller = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox),scroller,TRUE,TRUE,0);

	status =malloc(sizeof(statusbar));
	status->widget = gtk_statusbar_new();
	MW_SET("LEFT_STATUSBAR",status);
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(status->widget),FALSE);
	gtk_box_pack_start(GTK_BOX(vbox),status->widget,FALSE,FALSE,0);

	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_ICONS);
	MW_SET("LEFT_TOOLBAR",toolbar);
	gtk_box_pack_start(GTK_BOX(vbox),toolbar,FALSE,FALSE,0);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		N_("load Neutrino"), _("Neutrino Liste einlesen"),
		NULL, StockIcon(GTK_STOCK_OPEN),
		G_CALLBACK (but_loadNeutrino),MainWindow, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		N_("import Enigma"), _("Enigma Liste einlesen"),
		NULL, StockIcon(GTK_STOCK_OPEN),
		G_CALLBACK (but_loadEnigma),MainWindow, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		N_("import LCars"), _("LCars Liste einlesen"),
		NULL, StockIcon(GTK_STOCK_OPEN),
		G_CALLBACK (but_loadLcars),MainWindow, -1);

	gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("LEFT_TOOLBAR")), GTK_TOOLBAR_BOTH);

	//============= Linke Liste
	//=============
	store = gtk_list_store_new(LEFT_LIST__N_COLUMNS,
			G_TYPE_INT,GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_STRING, G_TYPE_POINTER );
 treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
//	gtk_tree_view_set_search_column (GTK_TREE_VIEW(treeview), LEFT_LIST__CHANNEL );
//	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview),TRUE);
//	gtk_signal_connect(GTK_OBJECT(treeview),"start-interactive-search",
//		G_CALLBACK(search),(gpointer) 0);
//	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW (treeview),
//		SearchEqualFunc,
//		NULL,
//		 NULL);
//		search_destroy);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(selection ,GTK_SELECTION_MULTIPLE);
	MW_SET("LEFT_LIST_STORE",store);
	MW_SET("LEFT_TREEVIEW",treeview);

	gtk_container_add(GTK_CONTAINER(scroller),treeview);

	// drag&drop Source.
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview),
		GDK_BUTTON1_MASK, target,targets, GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(treeview),"drag_data_get",
		G_CALLBACK(tree_drag_data_get),(gpointer) 0);

	// cell Renderer
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,"text", LEFT_LIST__DISEQC, NULL);
	MW_SET("LEFT_VIEW_DISEQC", column );
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,20);

//	gtk_tree_view_column_set_sort_column_id(column,LEFT_LIST__DISEQC);
//	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),
//		LEFT_LIST__DISEQC,treeview_sort, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&sat_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_tree_view_column_set_widget(column, GTK_WIDGET(image));
	gtk_widget_show (image);

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("",
		renderer, "pixbuf", LEFT_LIST__SERVICE, NULL);
	MW_SET("LEFT_VIEW_SERVICE", column );
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,20);
//	gtk_tree_view_column_set_sort_column_id(column,LEFT_LIST__SERVICE);
//	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),
//		LEFT_LIST__SERVICE,treeview_sort, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&service_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_tree_view_column_set_widget(column, GTK_WIDGET(image));
	gtk_widget_show (image);


	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Bouquet",
		renderer, "text", LEFT_LIST__BOUQUET, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,100);
//	gtk_tree_view_column_set_sort_column_id(column,LEFT_LIST__BOUQUET);
//	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),
//		LEFT_LIST__BOUQUET,treeview_sort, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	MW_SET("LEFT_VIEW_BOUQUET", column );

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Channel"),
		renderer, "text", LEFT_LIST__CHANNEL, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,100);
	
	gtk_tree_view_column_set_sort_column_id(column,LEFT_LIST__CHANNEL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),
 		LEFT_LIST__CHANNEL,treeview_sort, (gpointer) LEFT_LIST__CHANNEL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	MW_SET("LEFT_VIEW_CHANNEL", column );

	return vbox;
}


GtkWidget *create_right_list_panel(){
	statusbar *status;
	GtkCellRenderer* renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GtkWidget *scroller;
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkWidget *vbox, *hbox, *hboxb;
	GtkWidget *toolbar;
	GtkWidget *image, *arrow;
	GtkWidget *button, *label;
	GdkPixbuf *icon;
	GtkTooltips *button_bar_tips;
	GtkTreeSelection *selection;

	button_bar_tips = gtk_tooltips_new ();
	vbox = gtk_vbox_new(FALSE,0);

	gtk_box_pack_start(GTK_BOX(vbox),hbox = gtk_hbox_new(FALSE,0), FALSE, FALSE, 0);
	button = gtk_button_new();
	gtk_widget_set_usize(button, 30, 24);
	gtk_container_add(GTK_CONTAINER(button),StockIcon(GTK_STOCK_ADD));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button, _("add a Bouquet"), "");
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_addBouquet), (gpointer) button);
	MW_SET("BUT_ADD_BOUQUET", button);

	button = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(button), hboxb = gtk_hbox_new(FALSE,0));
	arrow = StockIcon(GTK_STOCK_INDEX);
	gtk_widget_set_usize(arrow,20, 15);	
	gtk_box_pack_end(GTK_BOX(hboxb),arrow, FALSE, FALSE, 0);
	label = gtk_label_new (_("<Bouquets are empty>"));
	gtk_box_pack_start(GTK_BOX(hboxb), label, FALSE, FALSE, 0);
	gtk_widget_set_usize(label,140, 15);
	gtk_box_pack_start(GTK_BOX(hboxb), gtk_label_new (""),TRUE, TRUE, 0);
	gtk_tooltips_set_tip (GTK_TOOLTIPS (button_bar_tips), button,
		_("switch to Bouquet-Viewmode"), "");
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(but_toggleViewMode), NULL);
//	gtk_widget_set_sensitive(button,FALSE);
	MW_SET("TOGGLE_VIEW",button);
	MW_SET("TOGGLE_LABEL",label);

	//============= Rechte Liste
	//============= CHANNELS
	scroller = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox),scroller,TRUE,TRUE,0);
	MW_SET("CHANNEL_SCROLLER",scroller);

	store = gtk_list_store_new(RIGHT_CHANNEL_LIST__N_COLUMNS,
		G_TYPE_INT, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER );
	MW_SET("RIGHT_LIST_CHANNEL_STORE",store);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	g_signal_connect(G_OBJECT(treeview), "button_press_event", G_CALLBACK(showChannelData),NULL);
	// drag&drop Source.
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview),
		1,target,targets, GDK_ACTION_MOVE);
	gtk_signal_connect(GTK_OBJECT(treeview),"drag_data_get",
		G_CALLBACK(tree_drag_data_get),(gpointer) 1);
	// drag&drop Destination.
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(treeview),
		target,targets, GDK_ACTION_MOVE);
	gtk_signal_connect(GTK_OBJECT(treeview),"drag_data_received",
		G_CALLBACK(tree_drag_data_received),(gpointer)1);

	MW_SET("RIGHT_CHANNEL_TREEVIEW",treeview);
	gtk_container_add(GTK_CONTAINER(scroller),treeview);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(selection ,GTK_SELECTION_MULTIPLE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("",
		renderer, "text", RIGHT_CHANNEL_LIST__DISEQC, NULL);
	MW_SET("RIGHT_VIEW_DISEQC", column );
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,20);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&sat_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_tree_view_column_set_widget(column, GTK_WIDGET(image));
	gtk_widget_show (image);

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("",
		renderer, "pixbuf", RIGHT_CHANNEL_LIST__SERVICE, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column, 25);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&service_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_tree_view_column_set_widget(column, GTK_WIDGET(image));
	gtk_widget_show (image);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Nr.",
		renderer, "text", RIGHT_CHANNEL_LIST__NUMBER, NULL);
	MW_SET("RIGHT_VIEW_NUMBERS", column);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column, 30);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Channel"),
		renderer, "text", RIGHT_CHANNEL_LIST__CHANNEL, "editable", RIGHT_CHANNEL_LIST__EDITABLE, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column, 100);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (cell_edited), model);
	g_object_set_data(G_OBJECT (renderer), "viewmode", (gpointer) 1);

	//============= Rechte Liste
	//============= BOUQUETSS
	scroller = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox),scroller,TRUE,TRUE,0);
	MW_SET("BOUQUET_SCROLLER",scroller);
	store = gtk_list_store_new(RIGHT_BOUQUET_LIST__N_COLUMNS,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,G_TYPE_STRING,G_TYPE_BOOLEAN, G_TYPE_POINTER );

	MW_SET("RIGHT_LIST_BOUQUET_STORE",store);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	MW_SET("RIGHT_BOUQUET_TREEVIEW",treeview);
	gtk_container_add(GTK_CONTAINER(scroller),treeview);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
	g_signal_connect(G_OBJECT(treeview), "button_press_event", G_CALLBACK(dblClick),NULL);
	// drag&drop Source.
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview),
		1, target,targets, GDK_ACTION_MOVE);
	gtk_signal_connect(GTK_OBJECT(treeview),"drag_data_get",
		G_CALLBACK(tree_drag_data_get),(gpointer) 2);
	// drag&drop Destination.
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(treeview),
		target,1, GDK_ACTION_COPY|GDK_ACTION_MOVE);
	gtk_signal_connect(GTK_OBJECT(treeview),"drag_data_received",
		G_CALLBACK(tree_drag_data_received),(gpointer)2);

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes("",
			renderer, "active", RIGHT_BOUQUET_LIST__HIDDEN, NULL);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&hidden1_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_tree_view_column_set_widget(column, GTK_WIDGET(image));
	gtk_widget_show (image);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,25);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	g_object_set_data (G_OBJECT (renderer), "column", (gint *)RIGHT_BOUQUET_LIST__HIDDEN);
	g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (column_toggled), model);

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes("",
			renderer, "active", RIGHT_BOUQUET_LIST__LOCKED, NULL);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&locked1_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	g_object_unref(icon);
	gtk_tree_view_column_set_widget(column, GTK_WIDGET(image));
	gtk_widget_show (image);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,25);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	g_object_set_data(G_OBJECT(renderer), "column", (gint *)RIGHT_BOUQUET_LIST__LOCKED);
	g_signal_connect (G_OBJECT(renderer), "toggled", G_CALLBACK (column_toggled), model);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Bouquet"), renderer,
			"text", RIGHT_BOUQUET_LIST__BOUQUET, "editable", RIGHT_BOUQUET_LIST__EDITABLE, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(column,TRUE);
	gtk_tree_view_column_set_fixed_width(column,100);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview),column);
	g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (cell_edited), model);
	g_object_set_data(G_OBJECT (renderer), "viewmode", (gpointer) 0);


	status = malloc(sizeof(statusbar));
	status->widget = gtk_statusbar_new();
	MW_SET("RIGHT_STATUSBAR",status);
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(status->widget),FALSE);
	gtk_box_pack_start(GTK_BOX(vbox),status->widget,FALSE,FALSE,0);

	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_ICONS);
	MW_SET("RIGHT_TOOLBAR",toolbar);
	gtk_box_pack_start(GTK_BOX(vbox),toolbar,FALSE,FALSE,0);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("save All"), _("save Neutrino and Enigma"),
		NULL, StockIcon(GTK_STOCK_SAVE),
		G_CALLBACK (but_saveAllBouquet), MainWindow, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("save actual"), _("save actual Bouquet and LCars"),
		NULL, StockIcon(GTK_STOCK_SAVE),
		G_CALLBACK (but_saveAktBouquet), MainWindow, -1);

	gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar),
		_("save rtf"), _("save actual Bouquet as RichText (*.rtf)"),
		NULL, StockIcon(GTK_STOCK_SAVE),
		G_CALLBACK (but_saveAktAsRTF), MainWindow, -1);

	gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("RIGHT_TOOLBAR")), GTK_TOOLBAR_BOTH);

	return vbox;
}
	#include "pics/tv.xpm"
	#include "pics/nvod.xpm"
	#include "pics/radio.xpm"
	#include "pics/data.xpm"

void create_main_window(GtkWindow** return_window) {
	static GdkPixbuf *serviceType[N_SERVICES];
	GtkWidget* window;
	GtkWidget* hbox;

	window = gtk_widget_new(GTK_TYPE_WINDOW,
		"type",GTK_WINDOW_TOPLEVEL,
		"title","Gtk-Bouquet-Editor (GuckTux) V 0.85",
		"allow_grow",TRUE,
		"allow_shrink",FALSE,
		NULL);
	*return_window = GTK_WINDOW(window);
	g_signal_connect(window,"destroy", gtk_main_quit,NULL);
	g_signal_connect(G_OBJECT(MainWindow), "key_press_event", GTK_SIGNAL_FUNC(key_pressed), NULL);
	gtk_container_add(GTK_CONTAINER(window),hbox= gtk_hbox_new(FALSE,5));

/*
	GtkAccelGroup *accel_group;
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
	g_object_unref(accel_group);
*/

	gtk_box_pack_start(GTK_BOX(hbox),create_left_list_panel(),TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),create_button_panel(),FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),create_right_list_panel(),TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox),create_option_panel(),FALSE,FALSE,0);

	MW_SET("RIGHT_NODE_ROOT" , g_node_new("right_root"));
	MW_SET("LEFT_NODE_ROOT"  , g_node_new("left_root"));
	MW_SET("CLIPBOARD_LIST",NULL);
	MW_SET("TELNET_VIEW",   NULL);
	MW_SET("GLIST_PORTS",   NULL);
	MW_SET("UNDO_LIST",     NULL);
	MW_SET("REDO_LIST",     NULL);
	MW_SET("SERIAL_DIALOG", NULL);
 	serviceType[TV   ]= gdk_pixbuf_new_from_xpm_data((const char**)&tv_xpm);
	serviceType[NVOD ]= gdk_pixbuf_new_from_xpm_data((const char**)&nvod_xpm);
	serviceType[RADIO]= gdk_pixbuf_new_from_xpm_data((const char**)&radio_xpm);
	serviceType[DATA ]= gdk_pixbuf_new_from_xpm_data((const char**)&data_xpm);
	MW_SET("SERVICE_TYPE_GFX", &serviceType);
}


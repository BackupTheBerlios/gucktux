/***************************************************************************
                          main.c  -  description
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#ifndef WIN32
#include <langinfo.h>
#endif
#include "gsatedit.h"
#include "listen.h"
#include "tools.h"

GtkTargetEntry target[]={
	{ "CHANNEL_SELECTION", GTK_TARGET_SAME_APP, DRAGDROP_TARGET_CHANNEL },
	{ "BOUQUET_SELECTION", GTK_TARGET_SAME_APP, DRAGDROP_TARGET_BOUQUET }};
guint targets=sizeof(target)/sizeof(GtkTargetEntry);
 

void ReadConfig(){
	gint width;
	gint height;
	gboolean show;
	gchar buffer[20];
	
	width = atoi(cfg_read("geometry","window width" ,"800"));
	height= atoi(cfg_read("geometry","window height","600"));
	gtk_window_set_default_size(MainWindow,width,height);
	width = atoi(cfg_read("geometry","window posx" ,"10"));
	height= atoi(cfg_read("geometry","window posy" ,"10"));
	gtk_window_move(MainWindow, width, height);

	show = atoi(cfg_read("listview","show header" ,"1"));
	gtk_toggle_button_set_active(MW_GET("OPT_SHOW_HEADER"), show);
	gtk_tree_view_set_headers_visible(MW_GET("LEFT_TREEVIEW"), show);
	gtk_tree_view_set_headers_visible(MW_GET("RIGHT_CHANNEL_TREEVIEW"),show);
	gtk_tree_view_set_headers_visible(MW_GET("RIGHT_BOUQUET_TREEVIEW"),show);
	show = atoi(cfg_read("listview","show diseqc" ,"1"));
	gtk_toggle_button_set_active(MW_GET("OPT_SHOW_DISEQC"), show);
	gtk_tree_view_column_set_visible(MW_GET("LEFT_VIEW_DISEQC"), show);
	gtk_tree_view_column_set_visible(MW_GET("RIGHT_VIEW_DISEQC"),show);
	show = atoi(cfg_read("listview","show numbers" ,"1"));
	gtk_toggle_button_set_active(MW_GET("OPT_SHOW_NUMBERS"), show);
	gtk_tree_view_column_set_visible(MW_GET("RIGHT_VIEW_NUMBERS"),show);

	show = atoi(cfg_read("listview","show data services" ,"0"));
	gtk_toggle_button_set_active(MW_GET("OPT_READ_DATA"), show);
	show = atoi(cfg_read("listview","show fair channelnames" ,"1"));
	gtk_toggle_button_set_active(MW_GET("OPT_READ_WHITESPACE"), show);
	show = atoi(cfg_read("listview","show short channelnames" ,"0"));
	gtk_toggle_button_set_active(MW_GET("OPT_READ_SHORT"), show);

	sprintf(buffer, "%d", GTK_TOOLBAR_BOTH);
	show = atoi(cfg_read("toolbar","left toolbar style", buffer));
	gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("LEFT_TOOLBAR")), show);
	sprintf(buffer, "%d", GTK_TOOLBAR_BOTH);
	show = atoi(cfg_read("toolbar","middle toolbar style" , buffer));
	gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("MID_TOOLBAR")), show);
	sprintf(buffer, "%d", GTK_TOOLBAR_BOTH);
	show = atoi(cfg_read("toolbar","right toolbar style" , buffer));
	gtk_toolbar_set_style(GTK_TOOLBAR(MW_GET("RIGHT_TOOLBAR")), show);

	gtk_entry_set_text(MW_GET("OPT_NET_PATH"),cfg_read("network","path","/var/tuxbox/config/"));
	gtk_entry_set_text(MW_GET("OPT_NET_IP"),  cfg_read("network","ip",  "192.168.0.202"));
	gtk_entry_set_text(MW_GET("OPT_NET_USER"),cfg_read("network","user","root"));
	gtk_entry_set_text(MW_GET("OPT_NET_PASS"),cfg_read("network","password", "dbox2"));
	gtk_entry_set_text(MW_GET("OPT_SERIAL_PORT"),cfg_read("network","serial port", 
 		(gchar*)gtk_entry_get_text(MW_GET("OPT_SERIAL_PORT"))));
}

gboolean freeMem(){
	clear_node (MW_GET("RIGHT_NODE_ROOT"));
	clear_node (MW_GET("LEFT_NODE_ROOT"));
	g_list_free(MW_GET("GLIST_PORTS"));
	clearUndo();
	clearRedo();
	return FALSE;
}

int main(int argc,char** argv) {
	gboolean language = FALSE;
	setlocale(LC_ALL,"");
	bindtextdomain("gsatedit","./po/");
	textdomain("gsatedit");
	#ifndef WIN32
	if ((strstr(nl_langinfo(CODESET), "ISO-8859-1"))  || //Debian 3.0, Suse   8.1
			(strstr(nl_langinfo(CODESET), "ISO-8859-15")) || //Debian 3.0, Suse   8.1
			(strstr(nl_langinfo(CODESET), "UTF-8"))) language = TRUE; //   RedHat 8.0
	#else
	language = TRUE;
	#endif
	gtk_init (&argc,&argv);
	create_main_window(&MainWindow);
	g_signal_connect(G_OBJECT(MainWindow), "delete_event", G_CALLBACK(freeMem), NULL);

	ReadConfig();
	gtk_widget_show_all(GTK_WIDGET(MainWindow));
	gtk_widget_hide(MW_GET("BOUQUET_SCROLLER"));

	if (language == FALSE)
	{
		GtkWidget* dialog;
		dialog = gtk_message_dialog_new (MainWindow,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
			_("This programm is based on Gtk-2 and needs at least the\n"
			  "ISO-8859-1 codeset.\n"
			  "If you want me to be stable, your environment needs an upgrade.\n\n"
			  "If you don't understand this message - contact us.\n"));
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
		gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	clear_right_listview();

	g_thread_init(NULL);
	gtk_main();
	return 0;
}

/*
g_thread_create(thTest, NULL, FALSE, NULL);
gpointer thTest(gpointer data){
	gint i =0;
	while(1) printf("%d\n", i++);

}
*/


/***************************************************************************
                          lcars.c  -  description
                             -------------------
    begin                : 31.1.2001
    copyright            : (C) 2001 by Abadon
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gsatedit.h"
#include "tools.h"
#include "listen.h"

////////////////////////////////////////////////////////////////////////////////
//  16-bit Peek & Poke.
////////////////////////////////////////////////////////////////////////////////
void poke(char *dest,int wert){
	dest[0]= wert/256;
	dest[1]= wert &0xff;
}

unsigned int peek(unsigned char *source){
	return source[0]*256 + source[1];
}

////////////////////////////////////////////////////////////////////////////////
// Sendernamen dekodieren (kurze Namen, führende Whitespaces, ...)
////////////////////////////////////////////////////////////////////////////////
void decode(guchar *dest, guchar *source){
	#define START 0x86  // shortNames START
	#define STOP  0x87  // shortNames STOP
	static guchar *merker;
	// skip Whitespaces.
	while (*source == 0x05 || *source==' ') source++;
	if ((!GTK_TOGGLE_BUTTON(MW_GET("OPT_READ_WHITESPACE"))->active)){
		while (*source=='.' || *source==',' || *source=='_') source++;
	}
	// if there is no channelName -> make one.
	if (*source=='\n' || *source=='\0'){
	 strcpy(dest,_("unknown"));
	 return;
	}
	source[MAX_TXT_LEN]=0;
	// decode shortNames.
	if ((!GTK_TOGGLE_BUTTON(MW_GET("OPT_READ_SHORT"))->active)){
		while (*source && *source != '\n'){
			*dest = *source++;
			if ((*dest>31) && (*dest!=START) && (*dest!=STOP)) dest++;
  	}
		*dest='\0';
		return;
	}
	merker=dest;
	while(*source!= '\n'){
		if (*source == START){
			source++;
			while (*source == START) source++; // für Sender mit >1 Startzeichen hintereinander
			dest=merker;
			while (*source!= STOP  && *source!= '\n') *dest++ = *source++;
			while (*source!= START && *source!= '\n') source++;
			merker=dest;
		}
		else *dest++ = *source++;
	}
	*dest='\0';
}

////////////////////////////////////////////////////////////////////////////////
//  load LCars.
////////////////////////////////////////////////////////////////////////////////
void but_loadLcars(gpointer callback_data, guint  callback_action, GtkWidget *widget){
	FILE *stream;
	GNode *node_bouquet, *node_channel = NULL;
	gchar txtBuffer[200];
	channelEntry *channel;
	bouquetEntry *bouquet;
	
	if (!(stream = fopen(get_path(LCARS_SERV_NAME), "rb"))){
		GtkWidget* dialog;
		sprintf(txtBuffer,_("File '%s' could not be opened."),LCARS_SERV_NAME);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	fread(&txtBuffer,64,1,stream);
	if (!strFind(txtBuffer,"DVSO"))
	{
		GtkWidget* dialog;
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK,txtIn(_("unknown file format")));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	rewind(stream);

	GTK_LIST_STORE(MW_GET("LEFT_LIST_STORE"))->sort_column_id = -2; 	// switch off sorting.
	clear_left_listview();
	bouquet = malloc(sizeof(bouquetEntry));
	node_bouquet = g_node_append(MW_GET("LEFT_NODE_ROOT"), g_node_new(bouquet));
	strcpy(bouquet->bouquetName, _("myBouquet"));
	bouquet->hidden = 0;
	bouquet->locked = 0;
	while(1){ // alle Bouquets einlesen.
		if (fread(&txtBuffer,64,1,stream)!=1) break;
		if ((!GTK_TOGGLE_BUTTON(MW_GET("OPT_READ_DATA"))->active) && (txtBuffer[27]&0xff) >11) continue;
		channel = malloc(sizeof(channelEntry));
		node_channel = g_node_insert_after(node_bouquet, node_channel, g_node_new(channel));
		channel->serviceID    = peek(txtBuffer+ 4);
		channel->frequency    = peek(txtBuffer+ 8)*1000;
		channel->symbolRate   = peek(txtBuffer+10)*1000;
		channel->fec          = txtBuffer     [12]&0xff;
		channel->polarisation = txtBuffer     [14]&0xff;
		channel->diseqc       = txtBuffer     [15]&0xff;
		channel->serviceType  = txtBuffer     [27]&0xff;
		channel->transportID  = peek(txtBuffer+30);
		txtBuffer[56]='\n'; // textEnd SenderName.
//		strcpy(channel->channelName, txtBuffer+32);
		decode(channel->channelName, txtBuffer+32);
		channel->onid         = peek(txtBuffer+62);
	}
	gtk_widget_grab_focus(GTK_WIDGET(MW_GET("OPT_READ_SHORT"))); // unfocus search entry.
	fill_left_listview();
}

////////////////////////////////////////////////////////////////////////////////
//  Kanalliste im Lcars-Format speichern.
////////////////////////////////////////////////////////////////////////////////
void saveLCars(){
	FILE *stream;
	GNode *node_channel;
	gchar txtBuffer[200];
	channelEntry *channel;

	node_channel = g_node_first_child (MW_GET("AKT_BOUQUET"));
	if (!node_channel) return;
	if (!(stream = fopen(get_path(LCARS_SERV_NEW), "wb"))){
		GtkWidget* dialog;
		sprintf(txtBuffer,_("File '%s' could not be opened."),LCARS_SERV_NEW);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		clear_left_listview();
		return;
	}
	while (node_channel){
		channel = (channelEntry *) node_channel->data;
		memset(&txtBuffer, 0, 64);
		strcpy(txtBuffer,"DVSO");
		strncpy(txtBuffer+32, channel->channelName,24);
		poke(txtBuffer+ 4, channel->serviceID);
		poke(txtBuffer+ 8, channel->frequency/1000);
		poke(txtBuffer+10, channel->symbolRate/1000);
		poke(txtBuffer+16,0x1fff); // Video-PID
		poke(txtBuffer+18,0x1fff); // Audio-PID
		poke(txtBuffer+30, channel->transportID);
		poke(txtBuffer+62, channel->onid);
		txtBuffer[12]=channel->fec &0xff;
		txtBuffer[14]=channel->polarisation &0xff;
		txtBuffer[15]=channel->diseqc &0xff;
		txtBuffer[27]=channel->serviceType &0xff;
		txtBuffer[56]=0x03; // auto PID + PMT ein.
		fwrite(&txtBuffer,64,1,stream);
		node_channel = node_channel ->next;
	}
	fclose(stream);
}


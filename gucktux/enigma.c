/***************************************************************************
                          enigma.c  -  description
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gsatedit.h"
#include "tools.h"
#include "listen.h"

#define DEZ 0
#define HEX 1
#define CHR 2

////////////////////////////////////////////////////////////////////////////////
//  x-ten Eintrag aus Liste holen.
////////////////////////////////////////////////////////////////////////////////
int getValue(char *text, int nr, int basis){
	int endPos=1;
	while (nr > 0)
	{
		if (*text == 0x09) text++; // skip TAB.
		if (*text < '0' || *text == ':') nr--;
		text++;
	}
	while (text[++endPos] >= '0' && text[endPos] != ':');
	text[endPos]=0;
	if      (basis==DEZ) return atoi(text);
	else if (basis==HEX) return strtol(text, NULL, 16);
	return text[1];
}

////////////////////////////////////////////////////////////////////////////////
// Sendernamen dekodieren (kurze Namen, führende Whitespaces, ...)
////////////////////////////////////////////////////////////////////////////////
void decodeEnigma(guchar *dest, guchar *source){
	#define START 0x86  // shortNames START
	#define STOP  0x87  // shortNames STOP
	#define ENG1  0xc2  // Enigma command 
	#define ENG2  0xc3  // Enigma command 

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
			if (*dest == ENG2) *dest = *source++ +64; 
			if ((*dest>31) && (*dest!=START) && (*dest!=STOP) && (*dest!= ENG1)) dest++;
		}
		*dest='\0';
		return;
	}
	merker=dest;
	while(*source!= '\n'){
		if (*source == ENG1) source++;
		if (*source == START){
			source++;
			while (*source == START) source++; // für Sender mit >1 Startzeichen hintereinander
			dest=merker;
			while (*source!= STOP  && *source!= '\n' && *source!= ENG1) *dest++ = *source++;
			while (*source!= START && *source!= '\n') source++;
			merker=dest;
		}
		else *dest++ = *source++;
	}
	*dest='\0';
}

////////////////////////////////////////////////////////////////////////////////
//  Kanalliste im eDVB-Format laden.
////////////////////////////////////////////////////////////////////////////////
void but_loadEnigma(gpointer callback_data, guint callback_action, GtkWidget *widget){
	FILE *stream;
	guchar txtBuffer[100];
	guchar name[MAX_TXT_LEN+1];
	gint diseqc, transportID, frequenz, symbolRate, fec, polarity, onid, serviceID, serviceType;
	gint unknown;
	GNode *node_root, *node_bouquet, *node_channel;
	channelEntry *channel;
	bouquetEntry *bouquet;
	gint sumBouquets = 0;

	if (!(stream = fopen(get_path(EDVB_BOUQ_NAME), "rb")))
	{
		GtkWidget* dialog;
		sprintf(txtBuffer,_("File '%s' could not be opened."),EDVB_BOUQ_NAME);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	fgets(txtBuffer,BUFFER_SIZE,stream); // eDVB-Kennung.
	fgets(txtBuffer,BUFFER_SIZE,stream);
	if (!strFind(txtBuffer,"bouquets"))
	{
		GtkWidget* dialog;
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK,txtIn(_("unknown file format")));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		fclose(stream);
		return;
	}

	GTK_LIST_STORE(MW_GET("LEFT_LIST_STORE"))->sort_column_id = -2; 	// switch off sorting.
	clear_left_listview();
	node_root = MW_GET("LEFT_NODE_ROOT");
	node_bouquet = g_node_last_child(node_root);
	while(1) // alle Bouquets einlesen.
	{
		fgets(txtBuffer,BUFFER_SIZE,stream);	// Bouquet-Nr.
		if (strFind(txtBuffer,"end" )) break;
		bouquet = malloc(sizeof(bouquetEntry));
		sumBouquets++;
		node_bouquet = g_node_append(node_root, g_node_new(bouquet));
		fgets(txtBuffer,BUFFER_SIZE,stream); // Bouquet-Name.
		decodeEnigma(bouquet->bouquetName, txtBuffer);
		bouquet->hidden = 0;
		bouquet->locked = 0;
		node_channel = g_node_last_child (node_bouquet);
		while(1) // alle Sender einlesen.
		{
			fgets(txtBuffer,BUFFER_SIZE,stream);	// Senderdaten.
			if (strFind(txtBuffer,"/" )) break;
			channel = malloc(sizeof(channelEntry));
			node_channel = g_node_append(node_bouquet, g_node_new(channel));
			channel->serviceID   = getValue(txtBuffer,0,HEX);
			unknown              = getValue(txtBuffer,1,HEX);
			channel->transportID = getValue(txtBuffer,2,HEX);
			channel->onid        = getValue(txtBuffer,3,HEX);
			channel->serviceType = getValue(txtBuffer,4,DEZ);
			channel->frequency = 0;
		}
	}
	fclose(stream);
	//===================================
	// die services Datei einlesen und die Transponder hinzufügen.
	//===================================
	if (!(stream = fopen(get_path(EDVB_SERV_NAME), "rb")))
	{
		GtkWidget* dialog;
		sprintf(txtBuffer,_("File '%s' could not be opened."),EDVB_SERV_NAME);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	fgets(txtBuffer,99,stream); // "eDVB"-Kennung.
	fgets(txtBuffer,99,stream); // "Transponders"-Kennung.
	while(1)
	{
		fgets(txtBuffer,99,stream); // Zeile1
		if (strFind(txtBuffer,"end" )) break;
		unknown     = getValue(txtBuffer,0,HEX);		
		transportID = getValue(txtBuffer,1,HEX);
		onid        = getValue(txtBuffer,2,HEX);
		fgets(txtBuffer,99,stream); // txtBuffer[0]-> tab
		if (txtBuffer[1]=='s')
		{ // Stellen: 3-10 frequenz  12-19 symbolRate
			frequenz   = getValue(txtBuffer,1,DEZ);
			symbolRate = getValue(txtBuffer,2,DEZ);
			polarity   = getValue(txtBuffer,3,DEZ);
			fec        = getValue(txtBuffer,4,DEZ);
			//???      = getValue(txtBuffer,5,DEZ);
			diseqc     = getValue(txtBuffer,6,DEZ);
		}
		else
		{ // Stellen: 3-8 frequenz  10-15 symbolRate
			frequenz   = getValue(txtBuffer,1,DEZ) * 100;
			symbolRate = getValue(txtBuffer,2,DEZ) * 100;
			polarity=0;
			fec=0;
			diseqc=0;
		}
		fgets(txtBuffer,99,stream);
		// ******************************
		// jeden einzelnen Sender in der Liste mit den Transponder-Daten ergänzen.
		// ******************************
		node_bouquet = g_node_first_child(node_root);
		while (node_bouquet)
		{
			node_channel = g_node_first_child (node_bouquet);
			while (node_channel)
			{
				channel = (channelEntry *)node_channel->data;
				if ((transportID == channel->transportID) && (onid == channel->onid))
				{ // dieser Sender ist in den Bouquets. Also fehlende Daten ergänzen.
					channel->frequency = frequenz;
					channel->symbolRate = symbolRate;
					channel->polarisation = polarity;
					channel->fec = fec;
					channel->diseqc = diseqc;
				}
				node_channel = node_channel ->next;
			}
			node_bouquet = node_bouquet->next;
		}
	}
	//===================================
	// Sender-Daten einlesen.
	//===================================
	fgets(txtBuffer,99,stream); // "Services"-Kennung.
	while(1)
	{
		fgets(txtBuffer,99,stream); // Sender Daten
		if (strFind(txtBuffer,"end" )) break;
		serviceID   = getValue(txtBuffer,0,HEX);
		unknown     = getValue(txtBuffer,1,HEX);
		transportID = getValue(txtBuffer,2,HEX);
		onid        = getValue(txtBuffer,3,HEX);
		serviceType = getValue(txtBuffer,4,DEZ);
		fgets(txtBuffer,99,stream);  // Channel_Name
		decodeEnigma(name,txtBuffer);
		fgets(txtBuffer,99,stream);  // Bouquet_Name
		//===================================
		// jeden einzelnen Sender in der Liste mit den Transponder-Daten ergänzen.
		//===================================
		node_bouquet = g_node_first_child(MW_GET("LEFT_NODE_ROOT"));
		while (node_bouquet)
		{
			node_channel = g_node_first_child (node_bouquet);
			while (node_channel)
			{
				channel = node_channel->data;
				if ((transportID==channel->transportID)
					&& (onid==channel->onid) && (serviceID==channel->serviceID))
				{ // dieser Sender ist in den Bouquets. Also fehlende Daten ergänzen.
					strcpy(channel->channelName,name);
					channel->serviceType=serviceType;
					break;
				}
				node_channel = node_channel ->next;
			}
			node_bouquet = node_bouquet->next;
		}
	}
	fclose(stream);
	//******************************
	// Die Bouquets überprüfen. Wenn kein Eintrag bei (z.B.) Frequez vorhanden ist,
	// war der Sender nicht in der services enthalten -> Daten sind nicht komplett!
	// -> löschen. Ebenso wenn Datendienste nicht eingelsen werden sollten.
	//******************************
	node_bouquet = g_node_first_child(node_root);
	while (node_bouquet)
	{
		node_channel = g_node_first_child (node_bouquet);
		while (node_channel)
		{
			channel = node_channel->data;
			if ( (!channel->frequency) || ((!GTK_TOGGLE_BUTTON(MW_GET("OPT_READ_DATA"))->active)
				&& (getServicePic(channel->serviceType) == DATA)) )
			{ // Sender war in der Bouquets-datei, aber nicht in der Services -> Sender löschen.
				node_channel = remove_node(node_channel);
				continue;
			}
			node_channel = node_channel ->next;
		}
		if (!g_node_first_child (node_bouquet)) // bouquet now empty ? -> delete it.
		{
			node_bouquet = remove_node(node_bouquet);
			sumBouquets--;
			continue;
		}
		node_bouquet = node_bouquet->next;
	}
	gtk_widget_grab_focus(GTK_WIDGET(MW_GET("OPT_READ_SHORT"))); // unfocus search entry.
	fill_left_listview();
}


////////////////////////////////////////////////////////////////////////////////
//  ab in die eDVB Bouquets Datei.
////////////////////////////////////////////////////////////////////////////////
void saveEnigma(){
	FILE *stream;
	gchar txtBuffer[200];
	bouquetEntry *bouquet;
	channelEntry *channel;
	GNode *node_bouquet, *node_channel;
	int bouqNr =0;

	if (!(stream = fopen(get_path(EDVB_BOUQ_NEW), "wb")))
	{
		GtkWidget* dialog;
		sprintf(txtBuffer,_("File '%s' could not be opened."),EDVB_BOUQ_NEW);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	fputs("eDVB bouquets /2/ \n",stream);
	fputs("bouquets\n",stream);
	node_bouquet = g_node_first_child(MW_GET("RIGHT_NODE_ROOT"));
	while (node_bouquet)
	{
		bouquet = node_bouquet->data;
		node_channel = g_node_first_child (node_bouquet);
		if (node_channel)
		{ // Bouquet enthält Sender.
			sprintf(txtBuffer,"%d\n",bouqNr++);
			fputs(txtBuffer,stream);	// Bouquet Nr.
			sprintf(txtBuffer,"%s\n",bouquet->bouquetName);
			fputs(txtBuffer,stream);
			while (node_channel)
			{
				channel = node_channel->data;
				sprintf(txtBuffer,"%.4x:%.8x:%.4x:%.4x:%d\n", channel->serviceID, 0x00c00000,
					channel->transportID,channel->onid,channel->serviceType);
				fputs(txtBuffer,stream);
				node_channel = node_channel ->next;
			}
			fputs("/\n",stream);
		}
		node_bouquet = node_bouquet->next;
	}
	fputs("end\n",stream);
	fputs("Have a lot of fun!\n",stream);
	fclose(stream);
}

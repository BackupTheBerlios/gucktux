/***************************************************************************
                          neutrino.c  -  description
                             -------------------
    begin                : Die Sep 24 2002
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
#include <unistd.h>
#include "gsatedit.h"
#include "tools.h"
#include "listen.h"

////////////////////////////////////////////////////////////////////////////////
// XML-Sonderzeichen dekodieren.
////////////////////////////////////////////////////////////////////////////////
void XMLtoAsc(gchar *ascText, guchar *xmlText)
{
	gint len = MAX_TXT_LEN;
	gint shortToggle = -1;

	if (GTK_TOGGLE_BUTTON(MW_GET("OPT_READ_WHITESPACE"))->active){
		// führende Whitespaces überspringen.
		while (*xmlText==' ' || *xmlText=='.' || *xmlText==',' || *xmlText=='_') xmlText++;
	}
	// wenn kein Sendername -> Sendername = "unbekannt".
	if (*xmlText=='0' && *(xmlText+1)==0){
		strcpy(ascText,_("unknown\0"));
		return;
	}
	// XML dekodieren.
	while (*xmlText && len--){ // max. Textlänge beachten.
	  // UTF-8 code.
		if (*xmlText == 0xc3){
			xmlText++;
			*ascText++ = (*xmlText++) +64;
			continue;
		}
		if (*xmlText == 0xc5){
			xmlText+=2;
			continue;
		}
		//
		if (*xmlText=='&') switch (*(xmlText+1)){
			case 'l': // &lt;
				xmlText+=3;
				*xmlText='<';
				break;
			case 'g': // &gt;
				xmlText+=3;
				*xmlText='>';
				break;
			case 'a':
				if (*(xmlText+2) == 'p'){ // &apos;
					xmlText+=5;
					*xmlText='\'';
					break;
					}
				xmlText+=4; // &amp;
				*xmlText='&';
				break;
			case'q':
				xmlText+=5;
				*xmlText='\"';
				break;
			case'#':
				xmlText+=5;
				*xmlText =((*(xmlText-3))-'0')*100;	
				*xmlText+=((*(xmlText-2))-'0')*10;	
				*xmlText+=((*(xmlText-1))-'0');
				if (*xmlText == 194){ 
					shortToggle*= -1;
					xmlText++;
					continue;
				}
				break;
		}
		if (shortToggle == -1) *ascText++ = *xmlText++;
	}
	*ascText='\0';
}


////////////////////////////////////////////////////////////////////////////////
// XML-Sonderzeichen kodieren.
////////////////////////////////////////////////////////////////////////////////
gchar *ascToXML(guchar *ascText){
	static gchar txtBuffer[100];
	gchar *xmlText=txtBuffer;

	while (*ascText){
		switch (*ascText){
			case '\'':
				strcpy(xmlText,"&apos;");
				xmlText+=6;
				break;
			case '&':
				strcpy(xmlText,"&amp;");
				xmlText+=5;
				break;
			case '<':
				strcpy(xmlText,"&lt;");
				xmlText+=4;
				break;
			case '>':
				strcpy(xmlText,"&gt;");
				xmlText+=4;
				break;
			case '\"':
				strcpy(xmlText,"&quot;");
				xmlText+=6;
				break;
			default:
/*
				// old version
				if (*ascText > 127){
					sprintf(xmlText,"&#%.3d;",*ascText);
					xmlText+=6;
					break;
				}
*/
 // make UTF-8.
				if (*ascText > 127){
					*xmlText++ = 0xc3;
					*xmlText++ = *ascText -64;
					break;
				}

				*xmlText++=*ascText;
		}
		ascText++;
	}
	*xmlText='\0';
	return txtBuffer;
}

////////////////////////////////////////////////////////////////////////////////
// Gibt Zeiger auf den Inhalt des Schlüsselwortes (ohne Anführungszeichen) zurück.
////////////////////////////////////////////////////////////////////////////////
gchar *extractData(gchar *text, gchar *keyWord){
	static gchar buffer[100];
	gchar *ptmp= buffer;

	*ptmp='0'; *(ptmp+1)=0;			// default value = "0"
	text=strstr(text,keyWord);	// search for keyWord.
	if (!text) return buffer;		// keyWord not found.
	while (*text++ !='"');			// serch for first ".
	if (*text=='"') return buffer; // no text between "".
	while ((*ptmp++ = *text++)!='"');
	*(--ptmp) =0;
	return buffer;
}

////////////////////////////////////////////////////////////////////////////////
// Kanalliste im ZAPIT-Format laden.
////////////////////////////////////////////////////////////////////////////////
void load_file(GtkFileSelection *selector, gpointer file_selector);
void but_loadNeutrino(gpointer callback_data, guint callback_action, GtkWidget *widget){
	GtkWidget *file_selector = gtk_file_selection_new(_("select a Neutrino Channellist."));

	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION(file_selector));
	g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
		"clicked", G_CALLBACK (load_file),(gpointer) file_selector);
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
		"clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
		"clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);
	center_window(GTK_WINDOW(file_selector));
	gtk_widget_show (file_selector);
	gtk_file_selection_complete(GTK_FILE_SELECTION(file_selector),"*.xml");
}

void load_file(GtkFileSelection *selector, gpointer file_selector) {
	FILE *stream;
	gchar *pfad;
	gchar *filename;
	gchar txtBuffer[200];
	gchar bouquetName[MAX_TXT_LEN+1];
	bouquetEntry *bouquet;
	channelEntry *channel;
	GNode *node_root;
	GNode *node_bouquet;
	GNode *node_channel;
	gint sumBouquets = 0;
	gint diseqc, transportID, frequenz, symbolRate, fec, polarity, onid, serviceID, serviceType;
	gchar name[MAX_TXT_LEN+1];

	//******************************
	// load bouquets file & fill GNode.
	//******************************
	filename = (gchar*) gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));
	if (!(stream = fopen(filename, "rb"))){
		GtkWidget* dialog;
		sprintf(txtBuffer,_("Could not read: '%s' \n"),filename);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	fgets(txtBuffer,BUFFER_SIZE,stream); // xml Version.
	fgets(txtBuffer,BUFFER_SIZE,stream); // ZAPIT - ID.
	if (!strFind(txtBuffer,"<ZAPIT>" )){
		GtkWidget* dialog= gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
			txtIn(_("channel format unknown")));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		fclose(stream);
		return;
	}

	GTK_LIST_STORE(MW_GET("LEFT_LIST_STORE"))->sort_column_id = -2; 	// switch off sorting.
	clear_left_listview();
	fgets(txtBuffer,BUFFER_SIZE,stream);	// Bouquet-Kennung.
	if (!strFind(txtBuffer,"<BOUQUET" )){
		GtkWidget* dialog= gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
			txtIn(_("This is not a Bouquet File.\n"
							"Please select a bouquet-file like 'bouquets.xml'.")));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		fclose(stream);
		return;
	}   
	// ***** OK, this seems to be a bouquets file.
	fseek(stream,0,0); 
	fgets(txtBuffer,BUFFER_SIZE,stream); 	// xml Version.
	fgets(txtBuffer,BUFFER_SIZE,stream);	// ZAPIT - ID.

	node_root = MW_GET("LEFT_NODE_ROOT");
	while(1){ // read all Bouquets.
		fgets(txtBuffer,BUFFER_SIZE,stream);	// Bouquet-Daten.
		if (strFind(txtBuffer,"</ZAPIT>" )) break;
		bouquet = malloc(sizeof(bouquetEntry));
		sumBouquets++;
		node_bouquet = g_node_append(node_root, g_node_new(bouquet));
		XMLtoAsc(bouquetName, extractData(txtBuffer,"name"));
		strcpy(bouquet->bouquetName,bouquetName);
		bouquet->hidden = atoi(extractData(txtBuffer,"hidden"));
		bouquet->locked = atoi(extractData(txtBuffer,"locked"));
		node_channel = g_node_last_child (node_bouquet);
		while(1){ // read all channels.
			fgets(txtBuffer,BUFFER_SIZE,stream);
			if (strFind(txtBuffer,"</BOUQUET>" )) break;
			channel = malloc(sizeof(channelEntry));
			node_channel = g_node_append(node_bouquet, g_node_new(channel));
			channel->serviceID= strtol(extractData(txtBuffer,"serviceID"),NULL,16);
			XMLtoAsc(channel->channelName, extractData(txtBuffer,"name"));
			channel->onid= strtol(extractData(txtBuffer,"onid"),NULL, 16);
			channel->frequency = 0;
		}
	}
	fclose(stream);
	// ******************************
	// die services Datei einlesen und die Bouquets in verkette Liste mit diesen
	// Daten ergänzen.
	// ******************************
	pfad=filename+strlen(filename);
	while(*pfad!='\\' && *pfad!='/') pfad--;
	*++pfad='\0';
	strcpy(txtBuffer, filename);
	strcat(txtBuffer,ZAPIT_SERV_NAME);
	if (!(stream = fopen(txtBuffer, "rb"))){
		GtkWidget* dialog;
		strcat(txtBuffer, _(" not found."));
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		clear_left_listview();
		return;
	}
	fgets(txtBuffer,BUFFER_SIZE,stream); 	// xml Version.
	fgets(txtBuffer,BUFFER_SIZE,stream);	// ZAPIT-Kennung.

	while (1){ // alle Satelliten einlesen.
		fgets(txtBuffer,BUFFER_SIZE,stream);	// Sat / Kabel Daten.
		if (!strFind(txtBuffer,"<sat" ) && !strFind(txtBuffer,"<cable" )) break;	
		diseqc = atoi(extractData(txtBuffer,"diseqc"));
		while (1){ // alle Transponder einlesen.
			fgets(txtBuffer,BUFFER_SIZE,stream);	// Transponder
			if (strFind(txtBuffer,"</" )) break;
			transportID= strtol(extractData(txtBuffer,"transponder id"),NULL, 16);
			onid = strtol(extractData(txtBuffer,"onid"),NULL,16);
			frequenz= atoi(extractData(txtBuffer,"frequency"));
			symbolRate= atoi(extractData(txtBuffer,"symbol_rate"));
			fec= atoi(extractData(txtBuffer,"fec_inner"));
			polarity= atoi(extractData(txtBuffer,"polarization"));
			while(1){ // Alle Channels einlesen.
				gint test=0;
				fgets(txtBuffer,BUFFER_SIZE,stream);	// Kanaldaten.
				if (strFind(txtBuffer,"</" )) break;
				serviceID = strtol(extractData(txtBuffer,"service_id"),NULL,16);
				XMLtoAsc(name, extractData(txtBuffer,"name"));
				serviceType = strtol(extractData(txtBuffer,"service_type"),NULL,16);
				// ******************************
				// jeden einzelnen Sender in der Liste mit den neuen Daten ergänzen.
				// ******************************
				node_bouquet = g_node_first_child(node_root);
				while (node_bouquet){
					node_channel = g_node_first_child(node_bouquet);
					while (node_channel){
						channel = node_channel->data;
						if ((serviceID == channel->serviceID) && (onid == channel->onid))
						{ // dieser Sender ist in den Bouquets. Also fehlende Daten ergänzen.
							channel->serviceType=serviceType;
							channel->diseqc=diseqc;
							channel->transportID=transportID;
							channel->frequency=frequenz;
							channel->symbolRate=symbolRate;
							channel->fec=fec;
							channel->polarisation=polarity;
							test++;
						}
						node_channel = node_channel->next;
					}
					node_bouquet=node_bouquet->next;
				}
				// ******************************
				// Wenn der Sender aus den Services nicht in den Bouquets vorhanden war und die Liste
				// das komplette Bouquet enthält-> im Bouquet "*NEW*" eintragen.
				// ******************************
				if (!test && sumBouquets > 1){
					node_bouquet = g_node_last_child(node_root);
					bouquet = node_bouquet->data;
					if (strcmp(bouquet->bouquetName, "*NEW*")){
						bouquet = malloc(sizeof(bouquetEntry));
						node_bouquet = g_node_append(node_root, g_node_new(bouquet));
						strcpy(bouquet->bouquetName,"*NEW*");
						bouquet->hidden = FALSE;
						bouquet->locked = FALSE;
						sumBouquets++;
					}
					channel = malloc(sizeof(channelEntry));
					g_node_append(node_bouquet, g_node_new(channel));
					channel->serviceType=serviceType;
					channel->diseqc=diseqc;
					channel->transportID=transportID;
					channel->frequency=frequenz;
					channel->symbolRate=symbolRate;
					channel->fec=fec;
					channel->polarisation=polarity;
					XMLtoAsc(channel->channelName,name);
					channel->onid= onid;
					channel->serviceID= serviceID;
				}
			}
		}
	}
	fclose(stream);
	//******************************
	// Die Bouquets überprüfen. Wenn kein Eintrag bei (z.B.) Frequez vorhanden ist,
	// war der Sender nicht in der services enthalten -> Daten sind nicht komplett!
	// -> löschen. Ebenso wenn Datendienste nicht eingelsen werden sollten.
	//******************************
	node_bouquet = g_node_first_child(node_root);
	while (node_bouquet){
		node_channel = g_node_first_child (node_bouquet);
		while (node_channel){
			channel = node_channel->data;
			if ( (!channel->frequency) || ((!GTK_TOGGLE_BUTTON(MW_GET("OPT_READ_DATA"))->active)
				&& (getServicePic(channel->serviceType) == DATA)) )
			{ // Sender war in der Bouquets-datei, aber nicht in der Services -> Sender löschen.
				node_channel = remove_node(node_channel);
				continue;
			}
			node_channel = node_channel ->next;
		}
		if (!g_node_first_child (node_bouquet)){ // bouquet now empty ? -> delete it.
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
// Kanalliste im ZAPIT-Format speichern.
////////////////////////////////////////////////////////////////////////////////
void save_akt_bouquet(GtkFileSelection *selector, gpointer file_selector);
void save_all_bouquets();
void saveNeutrino(gint bouquet){
	GtkWidget *file_selector;
	if (bouquet){
		save_all_bouquets(); // alle Bouquets speichern.
		return;
	}
	file_selector = gtk_file_selection_new(_("save a Neutrino Channellist."));
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION(file_selector));
	g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
		"clicked", G_CALLBACK (save_akt_bouquet),(gpointer) file_selector);
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
		"clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);
	g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
		"clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector);
	center_window(GTK_WINDOW(file_selector));
	gtk_widget_show (file_selector);
	gtk_file_selection_complete(GTK_FILE_SELECTION(file_selector),"*.xml");
}

void save_all_bouquets(){
	FILE *stream;
	GNode *node_bouquet;
	GNode *node_channel;
	bouquetEntry *bouquet;
	channelEntry *channel;
	gchar txtBuffer[100];

	if (!(stream = fopen(get_path (ZAPIT_BOUQ_NEW), "wb"))){
		GtkWidget* dialog;
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
			txtIn(_("could not create bouquets-file")));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",stream);
	fputs("<zapit>\n",stream);	

	node_bouquet = g_node_first_child(MW_GET("RIGHT_NODE_ROOT"));
	while (node_bouquet){
		bouquet = node_bouquet->data;
		node_channel = g_node_first_child (node_bouquet);
		if (node_channel)	{ // Bouquet enthält Sender.
			sprintf(txtBuffer,"\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n",
				ascToXML(bouquet->bouquetName), bouquet->hidden, bouquet->locked);
			fputs(txtBuffer,stream);
			while (node_channel){
				channel = (channelEntry *) node_channel->data;
				sprintf(txtBuffer,"\t\t<channel serviceID=\"%.4x\" name=\"%s\" onid=\"%.4x\"/>\n",
					channel->serviceID, ascToXML(channel->channelName), channel->onid);
				fputs(txtBuffer,stream);
				node_channel = node_channel ->next;
			}
			fputs("\t</Bouquet>\n",stream);
		}
		node_bouquet = node_bouquet->next;
	}
	fputs("</zapit>\n",stream);	
	fclose(stream);
}

////////////////////////////////////////////////////////////////////////////////
// save the actual Bouquet.
////////////////////////////////////////////////////////////////////////////////
void save_akt_bouquet(GtkFileSelection *selector, gpointer file_selector) {
	FILE *stream;
	gchar *filename;
	gchar txtBuffer[200];
	bouquetEntry *bouquet;
	channelEntry *channel;
	GNode *node_bouquet, *node_channel;

	filename = (gchar*) gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));
	if (!(stream = fopen(filename, "wb"))){
		GtkWidget* dialog;
		sprintf(txtBuffer,_("Could not read: '%s' \n"),filename);
		dialog = gtk_message_dialog_new (NULL,0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,txtIn(txtBuffer));
		center_window(GTK_WINDOW(dialog));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",stream);
	fputs("<zapit>\n",stream);	

	node_bouquet = MW_GET("AKT_BOUQUET");
	bouquet = node_bouquet->data;
	node_channel = g_node_first_child (node_bouquet);
	if (node_channel){ // Bouquet enthält Sender.
		sprintf(txtBuffer,"\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n",
			ascToXML(bouquet->bouquetName), bouquet->hidden, bouquet->locked);
		fputs(txtBuffer,stream);
		while (node_channel){
			channel = node_channel->data;
			ascToXML(channel->channelName);
			sprintf(txtBuffer,"\t\t<channel serviceID=\"%.4x\" name=\"%s\" onid=\"%.4x\"/>\n",
				channel->serviceID, ascToXML(channel->channelName), channel->onid);
			fputs(txtBuffer,stream);
			node_channel = node_channel ->next;
		}
		fputs("\t</Bouquet>\n",stream);
	}
	fputs("</zapit>\n",stream);
	fclose(stream);
}


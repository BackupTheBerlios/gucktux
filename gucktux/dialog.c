/***************************************************************************
                          dialog.c  -  description
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <gdk/gdkkeysyms.h>
#include "dialog.h"
#include "gsatedit.h"
#include "tools.h"
#include "telnet.h"
#include "listen.h"

#ifndef WIN32
	#include <termios.h>
	gint fd;
#else
	#include <windows.h>
	HANDLE readThread = NULL, hCom = NULL;
#endif

#define TERMINAL_FONT "monospace 10"
gint inputTagSerial=0;


////////////////////////////////////////////////////////////////////////////////
//  Close serial-port window.
////////////////////////////////////////////////////////////////////////////////
void quitSerial(){
	#ifndef WIN32
	if (inputTagSerial) gdk_input_remove(inputTagSerial);
	close(fd);
	#endif
	inputTagSerial=0;
	gtk_widget_destroy(MW_GET("SERIAL_DIALOG"));
	MW_SET("SERIAL_DIALOG", NULL);
}

////////////////////////////////////////////////////////////////////////////////
//  Read serial port.
////////////////////////////////////////////////////////////////////////////////
#ifndef WIN32
void readSerial(gpointer data, gint source, GdkInputCondition condition){
	guchar text[256];
	gint len;
	if (!fd) return;
	len = read(source, text, sizeof(text));
	text[len] = 0;
	insertText(MW_GET("SERIAL_VIEW"), txtIn(text), TRUE);
}
#else
DWORD WINAPI readSerial(LPVOID lpvParam){
	DWORD dwRead;
	DWORD dwCommEvent;
	OVERLAPPED osReader ={0};
	gchar text[2]= {0,0};
	gchar chRead;

	if (!SetCommMask(hCom, EV_RXCHAR)) printf("Error setting communications event mask\n");
	while (inputTagSerial){
		if (!WaitCommEvent(hCom, &dwCommEvent, NULL)) break;
		do {
			if (!ReadFile(hCom, &chRead, 1, &dwRead, &osReader)) break;
			text[0] = chRead;
			if (chRead == '\b') insertText(MW_GET("SERIAL_VIEW"), "<bk>", TRUE);
			else insertText(MW_GET("SERIAL_VIEW"), txtIn(text), TRUE);
		} while (dwRead && inputTagSerial);
	}
	CloseHandle(readThread);
	CloseHandle(hCom);
	readThread = hCom = NULL;
	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//  Open serial-port window.
////////////////////////////////////////////////////////////////////////////////
void but_serial_dialog(gpointer button, GtkWidget *widget){
	GtkWidget *tempwid;
	GtkWidget *textView;
	GtkWidget *serial;

	if (MW_GET("SERIAL_DIALOG")){
		gtk_widget_hide(MW_GET("SERIAL_DIALOG"));
		gtk_widget_show(MW_GET("SERIAL_DIALOG"));
		return;
	}
	serial = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW (serial), ("Terminal"));
	gtk_widget_set_usize(serial, 600, 400);
	MW_SET("SERIAL_DIALOG", serial);

	tempwid = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER(GTK_DIALOG(serial)->vbox),tempwid);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tempwid), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	MW_SET("SERIAL_VIEW", textView = gtk_text_view_new());
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
	gtk_container_add (GTK_CONTAINER (tempwid),textView);
	gtk_widget_modify_font(textView,pango_font_description_from_string(TERMINAL_FONT));
	g_signal_connect(G_OBJECT(serial), "delete_event", G_CALLBACK(quitSerial), NULL);
	gtk_widget_show_all(serial);

	#ifndef WIN32
	{
/*
		struct termios options;   
		fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
		if (fd <0) {
			insertText(MW_GET("SERIAL_VIEW"), gtk_text_view_get_buffer(MW_GET("SERIAL_VIEW")),
				 txtIn(_("Cannot open serial port.\nPlease check your rights.")), FALSE);
			return;
		}
		fcntl(fd, F_SETFL,0);
		tcgetattr(fd, &options);
		cfsetispeed(&options, B9600);
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		options.c_cflag |= CLOCAL;
		options.c_cflag |= CREAD;
//		options.c_cflag |= CRTSCTS;
		tcsetattr(fd, TCSAFLUSH, &options);
		inputTagSerial = gdk_input_add(fd, GDK_INPUT_READ, readSerial, NULL);
*/


/*
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
 */

	static struct termios tios = {0, 0, CLOCAL|B115200|CS8|CREAD, 0, 0} ;
	cfsetispeed(&tios, B9600);
	fd = open((gchar*)gtk_entry_get_text(MW_GET("OPT_SERIAL_PORT")), O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd <0) {
		insertText(MW_GET("SERIAL_VIEW"), txtIn("Cound not open serial port.\n"
  		"Please check your rights."), TRUE);
		return;
	}
	tcsetattr(fd, TCSANOW, &tios);
	inputTagSerial = gdk_input_add(fd, GDK_INPUT_READ, readSerial, NULL);
	}
	#else
	{
		DWORD readThreadID = 0;
		DCB dcb;
		// Open the serial port.
		hCom = CreateFile((gchar*)gtk_entry_get_text(MW_GET("OPT_SERIAL_PORT")),
	  		GENERIC_READ, 0, 0,	OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		if (hCom == INVALID_HANDLE_VALUE) { printf("Invalid Handle!\n"); return; }
		memset(&dcb, 0, sizeof(dcb));
		if (!GetCommState(hCom, &dcb)) printf("Error getting dcb.");
		dcb.BaudRate =9600;
		dcb.Parity   =NOPARITY;
		dcb.StopBits =ONESTOPBIT;
		if (!SetCommState(hCom, &dcb)) printf("Error setting dcb.");
		inputTagSerial = 1;
		// Create a Thread for serial input.
		readThread = CreateThread(NULL, 0, readSerial, 0, 0, &readThreadID);
		SetThreadPriority(readThread, THREAD_PRIORITY_NORMAL);
	}
	#endif
}

////////////////////////////////////////////////////////////////////////////////
//  Print formatted Text.
////////////////////////////////////////////////////////////////////////////////
void insertText(GtkWidget *view, gchar *s, gboolean scroll){
	// <ax> -> attribute x.
	// <cx> -> color x.
	// <bk> -> backspace.

	GtkTextIter iter;
	GtkTextBuffer *buffer = GTK_TEXT_VIEW(view)->buffer;
	gtk_text_buffer_get_end_iter(buffer, &iter);

	while (!strncmp(s, "<bk>", 4)){
		GtkTextIter iterDel = iter;
		s+=4;
		if (!gtk_text_iter_backward_char(&iterDel)) break;
		gtk_text_buffer_delete(buffer, &iterDel, &iter);
	}

	gtk_text_buffer_get_end_iter(buffer, &iter);
	#define gtk_text_iwtbn gtk_text_buffer_insert_with_tags_by_name
	     if (!strncmp(s, "<ah>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "header"   , NULL);
	else if (!strncmp(s, "<ac>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "center"   , NULL);
  else if (!strncmp(s, "<ab>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "bold"     , NULL);
  else if (!strncmp(s, "<ai>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "italic"   , NULL);
	else if (!strncmp(s, "<al>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "underline", NULL);
	else if (!strncmp(s, "<c0>", 4)) gtk_text_buffer_insert(buffer, &iter, s+4, -1); // standard black.
	else if (!strncmp(s, "<c1>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "red"      , NULL);
	else if (!strncmp(s, "<c2>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "blue"     , NULL);
	else if (!strncmp(s, "<c3>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "cyan"     , NULL);
	else if (!strncmp(s, "<c4>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "green"    , NULL);
	else if (!strncmp(s, "<c5>", 4)) gtk_text_iwtbn(buffer, &iter, s+4, -1, "yellow"   , NULL);
	else gtk_text_buffer_insert(buffer, &iter, s, -1);
	#undef gtk_text_iwtbn

	if (scroll) gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(view),
		gtk_text_buffer_create_mark(buffer, NULL, &iter ,FALSE));
}

/*
////////////////////////////////////////////////////////////////////////////////
//  Repaint widget.
////////////////////////////////////////////////////////////////////////////////
void refresh_widget(GtkWidget *widget) {
	GdkRectangle rect;
	rect.x = rect.y = 0;
	rect.width = widget->allocation.width;
	rect.height= widget->allocation.height;
	gtk_widget_draw(widget, &rect);
}

////////////////////////////////////////////////////////////////////////////////
//  Is window already opend?
////////////////////////////////////////////////////////////////////////////////
int isActive(GtkWidget **Fenster){
	if (!(*Fenster)) return 0;
	gtk_widget_hide((*Fenster));   // Fenster in den Vordergrund
	gtk_widget_show((*Fenster));   // (kenne keinen toForeground Aufruf).
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Close window.
////////////////////////////////////////////////////////////////////////////////
// GTK setzt den Zeiger nicht von alleine auf NULL, also müssen alle
// gtk_widget_destroy diese Sub anspringen. (Auch der delete_event!).
void destroyWidget(GtkWidget **Fenster){
	gtk_widget_destroy((*Fenster));
	*Fenster = NULL;
}

*/
////////////////////////////////////////////////////////////////////////////////
// Show Channel infos.
////////////////////////////////////////////////////////////////////////////////
void makeEntry(char *titel, char *wert, gint posX, gint posY, gint breite, GtkWidget *fixed){
	#define y_offset 2
	GtkWidget *tempwid= gtk_label_new(titel);
	gtk_fixed_put(GTK_FIXED(fixed), tempwid, posX, posY + y_offset);
	gtk_widget_set_usize (tempwid, 92, 20);
	gtk_misc_set_alignment(GTK_MISC(tempwid), 1, 0);
	tempwid= gtk_entry_new();
	gtk_fixed_put(GTK_FIXED(fixed), tempwid, posX+95, posY);
	gtk_entry_set_text(GTK_ENTRY(tempwid), wert);
	gtk_widget_set_usize (tempwid, breite, 20);
	gtk_widget_set_sensitive(tempwid, FALSE);
	#undef y_offset
}

gboolean showChannelData(GtkTreeView *treeview, GdkEventButton *event, gpointer data){
	GtkWidget *okay_button, *fixed;
	GtkWidget *channelInfo;
	GtkTreePath *path;
	GtkTreeModel *model;
	GNode *node;
	channelEntry *channel;
	GtkTreeIter iter;
	gchar txtBuffer[20];
	gint i;
	gchar *polarity[]={"horizontal", "vertikal"};
	gchar *diseqc[]= {"Input1", "Input2","Input3","Input4"};
  gchar *type[]={	"Reserviert","TV","Radio","Teletext","NVOD ref.", "NVOD timeshift",
  								"Mosaic","PAL","Secam","D2-Mac","FM-Radio","Data"};
//	gchar *fec[]={"Auto","1/1", "1/2","2/3","3/4","5/6","7/8"};

	if (event->button != 3) return FALSE; // right mouse-button ?
	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), event->x , event->y,
 		 &path, NULL, NULL, NULL)) return FALSE;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(MW_GET("RIGHT_CHANNEL_TREEVIEW")));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, RIGHT_CHANNEL_LIST__NODE,  &node, -1);
	channelInfo = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(channelInfo), (_("Channel datas")));
	gtk_widget_set_usize(channelInfo, 400, 230);
//	gtk_window_set_position(GTK_WINDOW(changeSender), GTK_WIN_POS_MOUSE);
	okay_button = gtk_button_new_with_label("OK");
	gtk_container_add (GTK_CONTAINER(GTK_DIALOG(channelInfo)->action_area), okay_button);
	gtk_signal_connect_object(GTK_OBJECT(okay_button), "clicked",
		G_CALLBACK (gtk_widget_destroy), GTK_OBJECT(channelInfo));
	fixed = gtk_fixed_new ();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(channelInfo)->vbox), fixed);
	channel = node->data;
	makeEntry(_("Bouquet name:"),txtIn(((bouquetEntry*)node->parent->data)->bouquetName),
		 5 , 8, 150, fixed);
	makeEntry(_("Channel name:"), txtIn(channel->channelName),  5 ,30, 150,fixed);
	sprintf(txtBuffer,"%.8d",channel->frequency);
	makeEntry(_("Frequency:"), txtBuffer,  5 ,52 , 80,fixed);
	sprintf(txtBuffer,"%.8d",channel->symbolRate);
	makeEntry(_("Symbol Rate:"), txtBuffer,200,52 ,80,fixed);
	sprintf(txtBuffer,"%.4x",channel->transportID);
	makeEntry(_("Transport ID:"), txtBuffer, 5 ,74 ,80,fixed);
	sprintf(txtBuffer,"%.4x",channel->onid);
	makeEntry(_("Onid:"), txtBuffer,200 ,74 ,80,fixed);
	sprintf(txtBuffer,"%.4x",channel->serviceID);
	makeEntry(_("Service ID:"), txtBuffer, 5, 96 ,80,fixed);
	i= channel->serviceType; if (i> 11) i=11;
	makeEntry(_("Service Type:"),type[i],200, 96 ,80,fixed);
	makeEntry(_("Diseqc:"),diseqc[channel->diseqc], 5, 118 ,80,fixed);
	makeEntry(_("Polarisation:"),polarity[channel->polarisation],200, 118 ,80,fixed);
	gtk_widget_show_all(channelInfo);
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// Show key-settings info.
////////////////////////////////////////////////////////////////////////////////
#include "pics/F1.xpm"
#include "pics/F4.xpm"
#include "pics/F5.xpm"
#include "pics/F8.xpm"
#include "pics/plus.xpm"
#include "pics/minus.xpm"
#include "pics/del.xpm"
#include "pics/right.xpm"
#include "pics/backspace.xpm"
#include "pics/return.xpm"

void but_keyLayout(){
	GtkWidget *okay_button;// *fixed;
	GtkWidget *dialog, *tempwid, *fixed;
	GdkPixbuf* icon;
	GtkWidget* image;

//	if (isActive(&dialogTasten)) return; // Dialog schon offen?
	dialog = gtk_dialog_new();
	okay_button = gtk_button_new_with_label("Ok");
	gtk_window_set_title(GTK_WINDOW(dialog),txtOut(_("Key Layout")));

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&F1_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 48, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Show Help")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 72, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), gtk_hseparator_new ());

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&F4_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 48, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Insert")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 72, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),	gtk_label_new(
 		_("Insert selected Channels (left view) to marked position (right view).")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), gtk_hseparator_new ());

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&F5_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 48, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Copy")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 72, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(
 		_("Copy selected Channels (left view) to last position (right view).")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), gtk_hseparator_new ());

	icon = gdk_pixbuf_new_from_xpm_data((const char**)&return_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 48, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Copy")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 72, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(
 		_("Copy Channel from search Entry to right view.")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), gtk_hseparator_new ());

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&plus_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&minus_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 24, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Change position")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 72, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(
 		_("Move selection Up / Down a position.")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), gtk_hseparator_new ());
	
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&F8_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&del_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	icon = gdk_pixbuf_new_from_xpm_data((const char**)&backspace_xpm);
	image = gtk_image_new_from_pixbuf(icon);
	gtk_box_pack_start(GTK_BOX(tempwid),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Delete")) ,FALSE,FALSE,2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),tempwid = gtk_hbox_new(FALSE,2));
	gtk_box_pack_start(GTK_BOX(tempwid),fixed = gtk_fixed_new() ,FALSE,FALSE,0);
	gtk_widget_set_usize(fixed, 72, 0);
	gtk_box_pack_start(GTK_BOX(tempwid),gtk_label_new(_("Delete selection.")) ,FALSE,FALSE,2);
//	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_widget_show_all(dialog);
}

////////////////////////////////////////////////////////////////////////////////
// Execute telnet-macro.
////////////////////////////////////////////////////////////////////////////////
static void but_telnet_makro(GtkButton* button, gpointer func_data){
	FILE *stream;
	static gchar txtBuffer[256];
	gint makroNr= (gint) func_data+1;

	gtk_widget_grab_focus(MW_GET("TELNET_VIEW"));
	if (!(stream = fopen(get_path(MAKROS), "rb"))) return;
	while(makroNr){
		if (fgets(txtBuffer,99,stream)==NULL) break;
		if (txtBuffer[0]=='<') makroNr--;
	}
	while(1){
		if (fgets(txtBuffer,sizeof(txtBuffer),stream)==NULL) break;
		if (txtBuffer[0]=='<')  break;
		if (txtBuffer[0]=='#')  continue;
		if (txtBuffer[0]=='\n') continue;
		sendTelnet(txtBuffer); // send telnet string.
	}
	fclose(stream);
}

////////////////////////////////////////////////////////////////////////////////
// Telnet-Fenster Tastatureingabe.
////////////////////////////////////////////////////////////////////////////////
gboolean telnet_keyPress(GtkWidget * widget,  GdkEventKey * event,  gpointer data){
	static gchar userInput[256];
	static gint pos = 0;
	gint key = event->keyval;

	if ( key ==GDK_Up   || key == GDK_KP_Up 
		|| key ==GDK_Down || key == GDK_KP_Down) return TRUE;
	if ( key ==GDK_BackSpace) pos--;
	if ((key < 0x100) && (key != GDK_Return)) userInput[pos++] = key;
	if ((key == GDK_Return)||(key == GDK_KP_Enter)){
		userInput[pos++] = '\n';
		userInput[pos]=0;
		sendTelnet(userInput); // send telnet string.
		pos=0;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Telnet-Fenster öffnen.
////////////////////////////////////////////////////////////////////////////////
void but_telnet_dialog(gpointer button, GtkWidget *widget){
	GtkWidget *tempwid;
	GtkWidget *textView;
	GtkWidget *telnet;
	GtkTextBuffer *buffer;
	FILE *stream;
	gint i,j;
	gint anzMakros=0;
	gchar txtBuffer[100];
	GtkWidget *vbox[3];
	GtkWidget *butMakro[20];
	gchar butNamen[12][LEN_TELNET_BUT_NAME+1];

	if (MW_GET("TELNET_DIALOG")){
		gtk_widget_hide(MW_GET("TELNET_DIALOG"));
		gtk_widget_show(MW_GET("TELNET_DIALOG"));
		return;
	}
	telnet = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW (telnet), ("Telnet"));
	gtk_widget_set_usize(telnet, 600, 400);
//	gtk_window_set_position(GTK_WINDOW(telnet), GTK_WIN_POS_CENTER);
	MW_SET("TELNET_DIALOG",telnet);
	tempwid = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER(GTK_DIALOG(telnet)->vbox),tempwid);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tempwid), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	textView=gtk_text_view_new();
	MW_SET("TELNET_VIEW", textView);
	gtk_container_add (GTK_CONTAINER (tempwid),textView);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
	gtk_text_buffer_create_tag(buffer, "header","weight", PANGO_WEIGHT_BOLD,
		"size", 14 * PANGO_SCALE, NULL);
	gtk_text_buffer_create_tag(buffer, "blue",  "foreground", "blue", NULL);
	gtk_text_buffer_create_tag(buffer, "green", "foreground", "green", NULL);
	gtk_text_buffer_create_tag(buffer, "yellow","foreground", "yellow", NULL);
	gtk_text_buffer_create_tag(buffer, "red", "foreground", "red", NULL);
	gtk_text_buffer_create_tag(buffer, "cyan",  "foreground", "cyan", NULL);
	gtk_widget_modify_font(textView,pango_font_description_from_string(TERMINAL_FONT));
	g_signal_connect(G_OBJECT(textView), "key_press_event", G_CALLBACK(telnet_keyPress), NULL);
	g_signal_connect(G_OBJECT(telnet), "delete_event", G_CALLBACK(quitTelnet), NULL);

	for (i=0, j=0; i< 12;i++) sprintf(butNamen[i], "Makro %.2d",i+1);
	if ((stream = fopen(get_path(MAKROS), "rb"))){
		int pos;
		while(anzMakros < 12){
			if (fgets(txtBuffer,99,stream)==NULL) break;
			if (txtBuffer[0]!='<') continue;
			pos=0;
			while (pos< LEN_TELNET_BUT_NAME){
				if (txtBuffer[pos+1]=='>') break;
				butNamen[anzMakros][pos]=txtBuffer[pos+1];
				pos++;
			}
			butNamen[anzMakros++][pos]=0;
		}
	fclose(stream);
	}
	// ***************************************************************************
	for (j=0; j< 3;j++){
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(telnet)->action_area),vbox[j]= gtk_vbox_new(TRUE,5));
		gtk_widget_set_usize(vbox[j], 170, 0);
	}
	for (i=0, j=0; i< 12;i++){
		butMakro[i] = gtk_button_new_with_label(txtIn(butNamen[i]));
		if (i >= anzMakros) gtk_widget_set_sensitive(butMakro[i],FALSE);
		gtk_container_add (GTK_CONTAINER(vbox[j]), butMakro[i]);
		g_signal_connect(G_OBJECT(butMakro[i]), "clicked",
			G_CALLBACK(but_telnet_makro), (gpointer)i);
		if ((i&3)==3) j++;
	}


	gtk_widget_show_all(telnet);
	initTelnet(FALSE);
//	gtk_dialog_run(GTK_DIALOG(telnet));
//	gtk_widget_destroy(telnet);
}


////////////////////////////////////////////////////////////////////////////////
// Open a textbox with "ok" & "cancel" button.
////////////////////////////////////////////////////////////////////////////////
void textBox(gchar *title, gchar *filename ,gint width, gint height) {
	// if filename starts with '\n' -> this is not a file, but text to print.
	FILE *stream;
	GtkTextBuffer *buffer;
	GtkWidget *okay_button;
	GtkWidget *text1;
	GtkWidget *dialog, *tempwid;
	gchar txtBuffer[200];

//	if (isActive(&) return; // Dialog schon offen?
	dialog = gtk_dialog_new();
	okay_button = gtk_button_new_with_label("Ok");
	text1 = gtk_text_view_new ();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text1));

	gtk_text_buffer_create_tag(buffer, "header","weight", PANGO_WEIGHT_BOLD,
 		"size", 14 * PANGO_SCALE, NULL);
	gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag(buffer, "bold",  "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(buffer, "center",	"justification", GTK_JUSTIFY_CENTER, NULL);
	gtk_text_buffer_create_tag(buffer, "underline",	"underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_buffer_create_tag(buffer, "blue",  "foreground", "blue", NULL);
	gtk_text_buffer_create_tag(buffer, "green", "foreground", "green", NULL);
	gtk_text_buffer_create_tag(buffer, "yellow","foreground", "yellow", NULL);
	gtk_text_buffer_create_tag(buffer, "red", "foreground", "red", NULL);
	gtk_text_buffer_create_tag(buffer, "cyan",  "foreground", "cyan", NULL);
	g_signal_connect_swapped (GTK_OBJECT (okay_button), "clicked",
 		G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (dialog));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), okay_button);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
 		tempwid = gtk_scrolled_window_new(NULL, NULL));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tempwid),GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER(GTK_SCROLLED_WINDOW(tempwid)), text1);
	gtk_text_view_set_editable (GTK_TEXT_VIEW(text1),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text1),FALSE);

	if (!width)  width = 570;
	if (!height) height= 250;
	gtk_widget_set_usize (dialog, width, height);
//	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

	if (filename[0]=='\n') // if filename starts with '\n' -> this is not a file, but text to print.
	{
		int i=1,j=0;
		while(1){
			txtBuffer[j]=filename[i++];
			if (txtBuffer[j++]> 31) continue; // Enter, TextEnd.
			txtBuffer[j]=0;
			insertText(text1, txtIn(txtBuffer), FALSE);
			j=0;
			if (!filename[i-1]) break;
		}
	}else{
		if (!(stream = fopen(filename, "rb"))){
			sprintf(txtBuffer,_("<c1>File error: File '%s' could not be opened."),filename);
			insertText(text1, txtIn(txtBuffer), FALSE);
		}else{
			while(fgets(txtBuffer, 80,stream))	insertText(text1, txtIn(txtBuffer), FALSE);
			fclose(stream);
		}
	}
	gtk_window_set_title(GTK_WINDOW (dialog),title);
	gtk_widget_show_all(dialog);
}

////////////////////////////////////////////////////////////////////////////////
// InfoBox - Dialog.
////////////////////////////////////////////////////////////////////////////////
void InfoBox(gchar *title, gchar *text ,int breite, int hoehe) {
	GtkWidget *dialog, *label;
	dialog = gtk_dialog_new_with_buttons(title, MainWindow, GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);

	label = gtk_label_new("");
	gtk_label_set_markup (GTK_LABEL (label), txtIn(text));
	g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
 		G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (dialog));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
	gtk_widget_show_all (dialog);
}

////////////////////////////////////////////////////////////////////////////////
//  show Help.
////////////////////////////////////////////////////////////////////////////////
void but_help(){
	textBox("Hilfe","readme.txt",550,400);
}

////////////////////////////////////////////////////////////////////////////////
//  show FTP-log.
////////////////////////////////////////////////////////////////////////////////
void but_ftpLog(){
	textBox("FTP-Logfile","ftp_log.txt",0,0);
}

////////////////////////////////////////////////////////////////////////////////
// Show Info.
////////////////////////////////////////////////////////////////////////////////
void but_about(){
	InfoBox("Info",
	"\n <big><b>Gtk-Bouquets-Editor</b></big>"
	"\n <i>(C) 2002 by Abadon</i>"
	"\n <i>email : 3999@freenet.de</i>"
	"\n "
	"\n <i>ftp-Part:</i>"
	"\n <i>(C) 2002 by mycket</i>"
	"\n <i>email : mycket@freenet.de</i>"
	"\n "
	"\n This program is free software; you can redistribute it and/or modify it under the terms"
	"\n of the GNU General Public License as published by the Free Software Foundation;"
	"\n either version 2 of the License, or (at your option) any later version."
	"\n "
	"\n <span color=\"red\">"
 	" Dieses Programm ist noch im Betastadium. Die Benutzung erfolgt auf eigene Gefahr!</span>"
	"\n "
	"\n Sollte das Programm irgendwelche Rechte oder Personen verletzen, so gebt Bescheid. "
	"\n Wird sofort geaendert. Einen Anwalt einzuschalten ist nicht noetig."
	"\n "
	"\n <span color=\"blue\">Linux-Version made with KDevelop.</span>"
	"\n <span color=\"blue\">Win32-Version made with Dev-Cpp.</span>",
	0,360
	);
}



///////////////////////////////////////////////////////////////////////////////
// print debug-messages on console.
////////////////////////////////////////////////////////////////////////////////
void but_debug(gpointer callback_data, guint callback_action, GtkWidget *widget){
/*
	{
		GNode *node_bouquet;
		GNode *node_channel;
		bouquetEntry *bouquet;
		channelEntry *channel;

		printf("-----* Ausgabe linke Liste *-----\n");
		node_bouquet = g_node_first_child(MW_GET("LEFT_NODE_ROOT"));
		while (node_bouquet)
		{
			bouquet = (bouquetEntry *) node_bouquet->data;
			node_channel = g_node_first_child (node_bouquet);
			while (node_channel)
			{
				gint len;
				channel = (channelEntry *) node_channel->data;
				len = strlen(bouquet->bouquetName);
				for(;len < 20; len++) bouquet->bouquetName[len]=' ';
				bouquet->bouquetName[len+1]=0;
				len = strlen(channel->channelName);
				for(;len < 20; len++) channel->channelName[len]=' ';
				channel->channelName[len+1]=0;
				printf("%.15s # ", bouquet->bouquetName );
				printf("%.15s # ", channel->channelName);
				printf("%.4x # ", channel->serviceID);
				printf("%.4x # ", channel->onid);
				printf("%d # ", channel->frequency);
				printf("%d # ", channel->serviceType);
				printf("%d # ", channel->polarisation);
				printf ("\n");
				node_channel = node_channel ->next;
			}
			node_bouquet = node_bouquet->next;
		}
	}
*/
/*
	{
		GNode *node_bouquet;
		GNode *node_channel;
		bouquetEntry *bouquet;
		channelEntry *channel;
		printf("-----* Ausgabe rechte Liste *-----\n");
		node_bouquet = g_node_first_child(MW_GET("RIGHT_NODE_ROOT"));
		while (node_bouquet)
		{
			bouquet = node_bouquet->data;
//			node_channel = g_node_first_child (node_bouquet);
			node_channel = node_bouquet->children;
			while (node_channel)
			{
				gint len;
				channel = node_channel->data;
				len = strlen(bouquet->bouquetName);
				for(;len < 20; len++) bouquet->bouquetName[len]=' ';
				bouquet->bouquetName[len+1]=0;
				len = strlen(channel->channelName);
				for(;len < 20; len++) channel->channelName[len]=' ';
				channel->channelName[len+1]=0;
				printf("%.15s # ", txtIn(bouquet->bouquetName));
				printf("%.15s # ", txtIn(channel->channelName));
				printf("%.4x # ", channel->serviceID);
				printf("%.4x # ", channel->onid);
				printf ("\n");
				node_channel = node_channel ->next;
			}
			node_bouquet = node_bouquet->next;
		}
	}
*/

/*
	{
		GNode *node_channel;
		channelEntry *channel;
		printf("-----* Ausgabe CLIPBOARD Liste *-----\n");
		node_channel = g_node_first_child(MW_GET("CLIPBOARD_NODE"));
		while (node_channel)
		{
			gint len;
			channel = (channelEntry *) node_channel->data;
			len = strlen(channel->channelName);
			for(;len < 40; len++) channel->channelName[len]=' ';
			channel->channelName[len+1]=0;
			printf("%.15s # ",channel->channelName);
			printf("%.4x # ", channel->serviceID);
			printf("%.4x # ", channel->onid);
			printf ("\n");
			node_channel = node_channel ->next;
		}
	}
*/
/*
	{
		GList *undo_list = g_list_first(MW_GET("UNDO_LIST"));
		printf("\n-----* Ausgabe UNDO Buffer *-----\n");
		while (undo_list){
			undo *undo_entry = undo_list->data;
			switch (undo_entry->view) {
				case VIEW_CHANNELS: printf("CHANNEL  "); break;
				case VIEW_BOUQUETS: printf("BOUQUET  "); break;
				case VIEW_SETTINGS: printf("SETTINGS "); break;
			}
			switch (undo_entry->action) {
				case UNDO_RENAME:      printf("UNDO_RENAME "); break;
				case UNDO_DELETE:      printf("UNDO_DELETE "); break;
				case UNDO_COPY:        printf("UNDO_COPY   "); break;
				case UNDO_SWAP:			   printf("UNDO_SWAP   "); break;
				case UNDO_HIDE_LOCK:   printf("UNDO_HD_LK  "); break;
			}
			if (undo_entry->bouquet) printf("%s ", undo_entry->bouquet->bouquetName); else printf("(null) ");
			if (undo_entry->channel) printf("%s ", undo_entry->channel->channelName); else printf("(null) ");
			printf("pos: %d ", undo_entry->pos);
			printf("%d\n", undo_entry->jobNr);
			undo_list = undo_list->next;
		}
	}

	{
		GList *undo_list = g_list_first(MW_GET("REDO_LIST"));
		printf("\n-----* Ausgabe ReDO Buffer *-----\n");
		while (undo_list){
			undo *undo_entry = undo_list->data;
			switch (undo_entry->view) {
				case VIEW_CHANNELS: printf("CHANNEL  "); break;
				case VIEW_BOUQUETS: printf("BOUQUET  "); break;
				case VIEW_SETTINGS: printf("SETTINGS "); break;
			}
			switch (undo_entry->action) {
				case UNDO_RENAME:      printf("UNDO_RENAME "); break;
				case UNDO_DELETE:      printf("UNDO_DELETE "); break;
				case UNDO_COPY:        printf("UNDO_COPY   "); break;
				case UNDO_SWAP:			   printf("UNDO_SWAP   "); break;
				case UNDO_HIDE_LOCK:   printf("UNDO_HD_LK  "); break;
			}
			if (undo_entry->bouquet) printf("%s ", undo_entry->bouquet->bouquetName); else printf("(null) ");
			if (undo_entry->channel) printf("%s ", undo_entry->channel->channelName); else printf("(null) ");
			printf("pos: %d ", undo_entry->pos);
			printf("%d\n", undo_entry->jobNr);
			undo_list = undo_list->next;
		}
	}
*/

}


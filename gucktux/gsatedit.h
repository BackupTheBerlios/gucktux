/***************************************************************************
                          gsatedit.h  -  description
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

#ifndef GSATEDIT_H
#define GSATEDIT_H

#include <gtk/gtk.h>
#include <libintl.h>

#define MW_SET(name,value) g_object_set_data(G_OBJECT(MainWindow),name, (gpointer)value)
#define MW_GET(name) g_object_get_data(G_OBJECT(MainWindow),name)
#define  _(x)  gettext(x)
#define N_(x)  x

#define BUFFER_SIZE 256
#define MAX_TXT_LEN  40
#define LEN_TELNET_BUT_NAME 25

// defines for filenames
#define ZAPIT_FOLDER    "zapit/"
#define ZAPIT_SERV_NAME "services.xml"
#define ZAPIT_SERV_NEW  "_services.xml"
#define ZAPIT_BOUQ_NAME "bouquets.xml"
#define ZAPIT_BOUQ_NEW  "_bouquets.xml"
#define EDVB_FOLDER     "enigma/"
#define EDVB_SERV_NAME  "services"
#define EDVB_SERV_NEW   "_services"
#define EDVB_BOUQ_NAME  "bouquets"
#define EDVB_BOUQ_NEW   "_bouquets"
#define LCARS_FOLDER    "lcars/"
#define LCARS_SERV_NAME "lcars.dvb"
#define LCARS_SERV_NEW  "_lcars.dvb"
#define MAKROS          "makros.kat"
#define BOUQUET_TO_RTF  "myBouquet.rtf"
#define CFG_FILENAME    "gsatedit.cfg"
// defines for View-Mode toggle
#define CHANNEL_VIEW  (gpointer)+1
#define BOUQUET_VIEW  (gpointer)-1

enum {CLIPBOARD_MOVE, CLIPBOARD_COPY};
enum {UP, DOWN};
enum {VIEW_CHANNELS, VIEW_BOUQUETS, VIEW_SETTINGS};

enum {UNDO_COPY, UNDO_RENAME, UNDO_DELETE, UNDO_SWAP, UNDO_MOVE,
			UNDO_HIDE_LOCK,	UNDO_CHANNEL_VIEW, UNDO_BOUQUET_VIEW};

enum{
	LEFT_LIST__DISEQC,
	LEFT_LIST__SERVICE,
	LEFT_LIST__BOUQUET,
	LEFT_LIST__CHANNEL,
	LEFT_LIST__CHANNEL_NODE,
	LEFT_LIST__N_COLUMNS
};

enum{
	RIGHT_CHANNEL_LIST__DISEQC,
	RIGHT_CHANNEL_LIST__SERVICE,
	RIGHT_CHANNEL_LIST__NUMBER,
	RIGHT_CHANNEL_LIST__CHANNEL,
	RIGHT_CHANNEL_LIST__EDITABLE,
	RIGHT_CHANNEL_LIST__NODE,
	RIGHT_CHANNEL_LIST__N_COLUMNS
};

enum{
	RIGHT_BOUQUET_LIST__HIDDEN,
	RIGHT_BOUQUET_LIST__LOCKED,
	RIGHT_BOUQUET_LIST__BOUQUET,
	RIGHT_BOUQUET_LIST__EDITABLE,
	RIGHT_BOUQUET_LIST__NODE,
	RIGHT_BOUQUET_LIST__N_COLUMNS
};

typedef enum
{
	DRAGDROP_TARGET_CHANNEL,
	DRAGDROP_TARGET_BOUQUET
} DragDropTargetTypes;

enum {TV, NVOD, RADIO, DATA, N_SERVICES};

struct s_statusbar
{
	GtkWidget *widget;
	gint sumBouquets;
	gint sumSelected;
	gint sumChannels[N_SERVICES];
};
typedef struct s_statusbar statusbar;

struct s_bouquetEntry
{
	gchar bouquetName[MAX_TXT_LEN+1];
	gint hidden;
	gint locked;
};
typedef struct s_bouquetEntry bouquetEntry;

struct s_channelEntry
{
	gchar channelName[MAX_TXT_LEN+1];
	gint polarisation;
	gint serviceType;
	gint diseqc;
	gint serviceID;
	gint transportID;
	gint frequency;
	gint symbolRate;
	gint fec;
	gint onid;
};
typedef struct s_channelEntry channelEntry;

struct s_undo
{
	gint  view;   // Channel | Bouquet View
	gint  action; // copy | rename ...
	gint pos;
	gint  jobNr;
	channelEntry *channel;
	bouquetEntry *bouquet;
	GNode *node;
};
typedef struct s_undo undo;

struct s_clipboard
{
	gint action; // copy | move ...
	gint pos;
	gint source;
};
typedef struct s_clipboard clipboard;

gint undoJob, redoJob;
gint telnetConnected;
gboolean t_login;

void create_main_window(GtkWindow** );
GtkWindow* MainWindow;

#endif


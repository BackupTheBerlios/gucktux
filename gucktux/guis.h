/***************************************************************************
                          guis.h  -  description
                             -------------------
    begin                : Don Okt 3 2002
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
#ifndef GUIS_H
#define GUIS_H

void but_loadLcars(gpointer callback_data, guint  callback_action, GtkWidget *widget);
void but_loadEnigma(gpointer callback_data, guint callback_action, GtkWidget *widget);
void but_loadNeutrino(gpointer callback_data, guint  callback_action, GtkWidget *widget);
void saveLCars();
void saveEnigma();
void saveNeutrino(gint Bouquet);

#endif

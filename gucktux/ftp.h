/***************************************************************************
                          ftp.h  -  description
                             -------------------
    begin                : Die Jan 29 2002
    copyright            : (C) 2002 by mycket
    email                : mycket@freenet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FTP_H
#define FTP_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#ifndef WIN32
//#include <sys/types.h>
//#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#else
#include <winsock.h>
#endif

gchar buffer[2048];

void but_ftp_download();
void but_ftp_upload();

#endif

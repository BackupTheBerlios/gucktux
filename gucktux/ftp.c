/***************************************************************************
                          ftp.c  -  description
                             -------------------
    begin                : Thu Jan 24 2002
    copyright            : (C) 2002 by Mycket
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
#include "gsatedit.h"
#include "ftp.h"
#include "tools.h"
#include <string.h>
#include <stdlib.h>

#define FTP_PORT 21

#ifdef WIN32
#define read(x,y,z)  recv(x, y, z, 0)
#define write(x,y,z) send(x, y, z, 0)
#endif

////////////////////////////////////////////////////////////////////////////////
//  Text Output.
////////////////////////////////////////////////////////////////////////////////
void text_out(const gchar *buffer){
	gtk_entry_set_text(GTK_ENTRY(MW_GET("OPT_NET_STATUS")), buffer);
}

void write_log(gchar *logtext){
	FILE *ftp_log = fopen("ftp_log.txt", "a+");
	if(!ftp_log)	text_out("Logfile Error");
	fwrite(logtext, strlen(logtext),1, ftp_log);
	fclose(ftp_log);
}

////////////////////////////////////////////////////////////////////////////////
//  Error-Message
////////////////////////////////////////////////////////////////////////////////
gint ftpError(gint error){
	switch(error){
		case -1:
			text_out("FTP not available");
			return -1;
		case -2:
			text_out("Host not found");
			return -1;
		case -3:
			text_out("Socket Error");
			return -1;
		case -4:
			text_out("Can't connect to host");
			return -1;
		case -5:
			text_out("Wrong Password");
			return -1;
		case -6:
			text_out("Wrong Path entered");
			return -1;
		case -7:
			text_out("PASV Error");
			return -1;
		case -8:
			text_out("Data Socket Error");
			return -1;
		case -9:
			text_out("Data Conn Error");
			return -1;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//  Is telnet available?
////////////////////////////////////////////////////////////////////////////////
gint check_service(const gchar *buf){
	struct servent *service_entry;
	service_entry = getservbyname(buf, "tcp");

	if(!service_entry){
		sprintf(buffer, "<c1>Service not available!\n");
		write_log(buffer);
		return(-1);
	}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//  Read Message from Host.
////////////////////////////////////////////////////////////////////////////////
gint read_from_host(gint s, gchar *buf) {
	gint br;
	do{
		br = read(s, buf, 1);
		if (*buf == 13) br--;
		buf+= br;
	}while(br > 0);
	*buf = 0;
return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  Check path on dbox.
////////////////////////////////////////////////////////////////////////////////
gint check_path(gint cmd_socket){
	gchar *pathx =  (gchar *)gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_PATH")));
	if( pathx[strlen(pathx)-1] != 47) pathx[strlen(pathx)] = 47; // last char = '/'
	sprintf(buffer, "CWD %s\n", pathx);                    // Pruefen, ob einegeber Pfad OK
	send(cmd_socket, buffer, strlen(buffer), 0);
	read_from_host(cmd_socket, buffer);
	write_log(buffer);
	if(!strncmp(buffer, "\n550 ", 5)){
		close(cmd_socket);
		sprintf(buffer, "<c1>No path %s \n", pathx);
		write_log(buffer);
		return(-1);
		}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//  login.
////////////////////////////////////////////////////////////////////////////////
gint do_login(gint sock){
	sprintf(buffer,"user %s\n", gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_USER"))));
	write(sock, buffer, strlen(buffer));
	read_from_host(sock, buffer);
	write_log(buffer);
	sprintf(buffer,"pass %s\n", gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_PASS"))));
	write(sock, buffer, strlen(buffer));
	read_from_host(sock, buffer);
	write_log(buffer);

	if((strncmp(buffer,"\n530 ", 5)) == 0){
		close(sock);
		sprintf(buffer, "\n<c1>Wrong password!\n");
		write_log(buffer);
		return(-1);
	}
	//read_from_host(sock, buffer);              // Weitere Serverausgaben
	//write_log(buffer);
return(0);
}

////////////////////////////////////////////////////////////////////////////////
//  logout.
////////////////////////////////////////////////////////////////////////////////
void do_logout(gint cmd_socket){
	sprintf(buffer, "QUIT\n");                           // Abmelden
	write(cmd_socket, buffer, strlen(buffer));
	read_from_host(cmd_socket, buffer);
	write_log(buffer);
	#ifndef WIN32
	close(cmd_socket);
	#else
	closesocket(cmd_socket);
	#endif
}

////////////////////////////////////////////////////////////////////////////////
//  Create the data socket.
////////////////////////////////////////////////////////////////////////////////
int create_data_socket(int sock){
	gint p1, p2, d;
	gchar *port = buffer;
	gint data_port, data_socket;
	struct sockaddr_in data_conn;

	sprintf(buffer, "pasv\n");                     // Enter Passive Mode
	write(sock, buffer, strlen(buffer));
	read_from_host(sock, buffer);
	write_log(buffer);
	// extract Port from Servermessage.
	d =4; // skip IP-Adresse.
	while( d >0 && *port)	if (*port++ ==',') d--;

	if((sscanf(port, "%d,%d", &p1, &p2)) != 2){
		close(sock);
		sprintf(buffer, "\n<c1>Couldn't read Passive Mode Adress\n");
		write_log(buffer);
		return(-7);
	}

	data_port = ((p1 << 8) | p2);    // Datenport errechnen
	data_conn.sin_family = AF_INET;
	data_conn.sin_addr.s_addr = inet_addr(gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_IP"))));
	data_conn.sin_port = htons(data_port);
	sprintf(buffer, "\nCreating data connection to %s, Port %d",
 		inet_ntoa(data_conn.sin_addr), data_port);
	write_log(buffer);
	// Socket anlegen
	data_socket = socket(data_conn.sin_family, SOCK_STREAM, 0);
	if(sock == 0){
		sprintf(buffer, "<c1>Error creating data socket\n");
		write_log(buffer);
		return(-8);
	}

	if((connect(data_socket, (struct sockaddr *) &data_conn, sizeof(data_conn))) <0){
		close(sock);
		sprintf(buffer, "\n<c1>Couldn't create Data Connection");
		write_log(buffer);
		return(-9);
	} else {
		sprintf(buffer, "\n<c2>Success!");
	write_log(buffer);
	}
	return(data_socket);
}

////////////////////////////////////////////////////////////////////////////////
//  Create the Connection.
////////////////////////////////////////////////////////////////////////////////
gint create_connection(struct hostent *hp, gint port){
	struct sockaddr_in sa;
	gint sock;
	sa.sin_port = htons(FTP_PORT);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_IP"))));

	// Socket anlegen
	sock = socket(sa.sin_family, SOCK_STREAM, 0);
	if(!sock){
		sprintf(buffer, "<c1>Error creating socket\n");
		write_log(buffer);
		return(-3);
	}
	if((connect(sock, (struct sockaddr *) &sa, sizeof(sa))) <0){
		close(sock);
		sprintf(buffer, "<c1>Couldn't connect to host %s\n", inet_ntoa(sa.sin_addr));
		write_log(buffer);
		#ifndef WIN32
		if ((int)h_errno){
			sprintf(buffer, "<c1>%s \n", hstrerror((int)h_errno));
			write_log(buffer);
		}
		#endif
		return(-4);
	}
	read_from_host(sock, buffer); // dbox meldet sich
	write_log(buffer);
	return(sock);
}

////////////////////////////////////////////////////////////////////////////////
//  Download.
////////////////////////////////////////////////////////////////////////////////
gint do_download(gint cmd_socket, struct hostent *dbox, gchar *src_name, gchar *src_folder){
	gint data_socket;
	FILE *datei;

	send(cmd_socket, "TYPE i\n", strlen("TYPE i\n"), 0);     // Binärmodus
	read_from_host(cmd_socket, buffer);
	write_log(buffer);
	data_socket = create_data_socket(cmd_socket);  // Datenverbindung + Sendebefehl
	if (data_socket < 0) return (data_socket);
	sprintf(buffer, "RETR %s%s\n", src_folder, src_name);    // Anfordern
	write(cmd_socket, buffer, strlen(buffer));
	read_from_host(cmd_socket, buffer);
	write_log(buffer);
	if(!(strncmp(buffer, "\n550 ", 5))){
		sprintf(buffer, "\n<c1>No File %s%s found on server\n", src_folder, src_name);
		write_log(buffer);
		#ifdef WIN32
		write_log("\n ");
		closesocket(data_socket);
		#else
		close(data_socket);
		#endif
		return 0;
	}
	datei = fopen(src_name, "wb");                // bouquets.xml anlegen
	if(!datei){
		printf(buffer, "\n<c1>%s konnte nicht angelegt werden\n", src_name);
		write_log(buffer);
		#ifdef WIN32
		closesocket(data_socket);
		#else
		close(data_socket);
		#endif
		return 0;
	}
	while(1){                                    // empfangen und schreiben
		if (read(data_socket, buffer, 1) <= 0) break;
		fwrite(buffer, 1, 1, datei);
	}
	read_from_host(cmd_socket, buffer);
	//  Transfer complete
	if((strncmp(buffer, "\n226 ", 5)) == 0){
		sprintf(buffer, "\n<c2>Transfer complete\n" );
		write_log(buffer);
		#ifdef WIN32
		write_log("\n  ");
		#else
		if ((int)h_errno){
			sprintf(buffer, "<c1>%s \n", hstrerror((int)h_errno));
			write_log(buffer);
		}
		#endif
	}
	close(data_socket);
	fclose(datei);
	return(0);
}

gint ftp_down_main(){
	struct hostent *dbox;
	gint cmd_socket, dummy;
	write_log("\n \n<ab>--- Starting FTP Download from dbox ---\n");
	if((dummy = check_service("ftp"))) return(-1);
	cmd_socket = create_connection(dbox, FTP_PORT);
	if(cmd_socket < 0) return(cmd_socket);
	if((dummy = do_login(cmd_socket)))	return(-5);
	if((dummy = check_path(cmd_socket))) return(-6);
	write_log("\n");
	dummy = do_download(cmd_socket, dbox, ZAPIT_BOUQ_NAME, ZAPIT_FOLDER);
	if(dummy < 0) return(dummy);
	dummy = do_download(cmd_socket, dbox, ZAPIT_SERV_NAME, ZAPIT_FOLDER);
	if(dummy < 0) return(dummy);
	dummy = do_download(cmd_socket, dbox, LCARS_SERV_NAME, LCARS_FOLDER);
	if(dummy < 0) return(dummy);
	dummy = do_download(cmd_socket, dbox, EDVB_BOUQ_NAME, EDVB_FOLDER);
	if(dummy < 0) return(dummy);
	dummy = do_download(cmd_socket, dbox, EDVB_SERV_NAME, EDVB_FOLDER);
	if(dummy < 0) return(dummy);
	do_logout(cmd_socket);
	close(cmd_socket);
	return(0);
}

void but_ftp_download(){
	#ifdef WIN32
	WSADATA info;
	if (WSAStartup(MAKEWORD(1,1), &info)) {
 		text_out("Winsock error");
 		return;
	}
	#endif
	if (!ftpError(ftp_down_main())) text_out("FTP Download OK");
}

////////////////////////////////////////////////////////////////////////////////
//  Upload.
////////////////////////////////////////////////////////////////////////////////
gint do_upload(int cmd_socket, struct hostent *dbox, gchar *new, gchar *orig, gchar *folder){
	gchar z;
	gint data_socket;
	FILE *datei;

	send(cmd_socket, "TYPE i\n", strlen("TYPE i\n"), 0);     // Binärmodus
	read_from_host(cmd_socket, buffer);
	write_log(buffer);

	datei = fopen(new, "rb");                          // Open file
	if (!datei){
		sprintf(buffer, "\n<c1>%s konnte nicht geoeffnet werden\n", new);
		write_log(buffer);
		#ifndef WIN32
		if ((int)h_errno){
			sprintf(buffer, "<c1>%s \n", hstrerror((int)h_errno));
			write_log(buffer);
		}
		#endif
		return 0;
	}

	data_socket = create_data_socket(cmd_socket);  // Datenverbindung + Speicherbefehl
	if(data_socket < 0) return (data_socket);
	sprintf(buffer, "STOR %s%s\n", folder, orig);
	write_log("\n");
	write_log(buffer);
	write(cmd_socket, buffer, strlen(buffer));
	read_from_host(cmd_socket, buffer);
	if((strncmp(buffer, "\n550 ", 5)) == 0){
		sprintf(buffer, "\n<c1>550 No Path %s on server\n", folder);
		write_log(buffer);
	}
	else if((strncmp(buffer, "\n553 ", 5)) == 0) write_log("<c1>553 could not create file.\n");
	else
	{
		if (buffer[0] =='\n') write_log(buffer+1); else write_log(buffer);
		while(1){
			z = fgetc(datei);
			#ifndef WIN32
			if(z == EOF) break;
			write(data_socket, &z, 1);
			#else
			if(z == -1 ) break;
			send(data_socket, &z, 1, 0);
			#endif
		}
		#ifndef WIN32
		close(data_socket);
		#else
		closesocket(data_socket);
		#endif
		fclose(datei);
		read_from_host(cmd_socket, buffer);              // Transfer complete
		if((strncmp(buffer, "\n226 ", 5)) == 0){         // Nicht gefunden
			sprintf(buffer, "\n<c2>Transfer complete\n" );
			write_log(buffer);
			#ifdef WIN32
			write_log("\n  ");
			#else
			if ((int)h_errno){
				sprintf(buffer, "<c1>%s \n", hstrerror((int)h_errno));
				write_log(buffer);
			}
			#endif
		}
	}
	return(0);
}

gint ftp_up_main(){
	struct hostent *dbox;
	gint cmd_socket, dummy;
	write_log("\n \n<ab>--- Starting FTP Upload to dbox ---\n");
	if(check_service("ftp")) return(-1);
	cmd_socket = create_connection(dbox, FTP_PORT);
	if(cmd_socket < 0) return(cmd_socket);
	if((dummy = do_login(cmd_socket))) return(-5);
	if((dummy = check_path(cmd_socket))) return(-6);
	write_log("\n");
	dummy = do_upload(cmd_socket, dbox, ZAPIT_BOUQ_NEW, ZAPIT_BOUQ_NAME, ZAPIT_FOLDER);
	if(dummy < 0) return(dummy);
	dummy = do_upload(cmd_socket, dbox, LCARS_SERV_NEW, LCARS_SERV_NAME, LCARS_FOLDER);
	if(dummy < 0) return(dummy);
	dummy = do_upload(cmd_socket, dbox, EDVB_BOUQ_NEW, EDVB_BOUQ_NAME, EDVB_FOLDER);
	if(dummy < 0) return(dummy);
	do_logout(cmd_socket);
	close(cmd_socket);
	return(0);
}

void but_ftp_upload(){
	GNode *node_right_root = MW_GET("RIGHT_NODE_ROOT");
	#ifdef WIN32
	WSADATA info;
	if (WSAStartup(MAKEWORD(1,1), &info)){
		text_out("Winsock error");
		return;
	}
	#endif
	if (!(node_right_root->children) || !(node_right_root->children->children)){
		write_log("\n\n\n<c1>Can't upload. (No Bouquets were found in right listview)");
		text_out("Upload what ?");
		return;
	}
	but_saveAllBouquet(NULL, 0, NULL); // erst mal die Bouquets speichern, dann uploaden.
	if (!ftpError(ftp_up_main())) text_out("FTP Upload OK");
}

/***************************************************************************
                          telnet.c  -  description
                             -------------------
    begin                : Die Okt 1 2002
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

#include <string.h>
#ifndef WIN32
 #include <err.h>
 #include <sys/socket.h>
 #include <arpa/telnet.h>
#else
 #include <winsock.h>
 #include <errno.h>
#endif
#include "gsatedit.h"
#include "telnet.h"
#include "ftp.h"
#include "tools.h"
#include "dialog.h"


#define STR_USER  "Login:"    // when we can read this, we send the User-Name.
#define STR_PASS  "Password:" // when we can read this, we send the Password.
#define STR_LOGIN "BusyBox"   // when we can read this, we are connected.
#define MY_STATE_WILL           0x01
#define MY_WANT_STATE_WILL      0x02
#define MY_STATE_DO             0x04
#define MY_WANT_STATE_DO        0x08
#ifdef WIN32
 #define ENOBUFS  WSAENOBUFS
 #define EWOULDBLOCK  WSAEWOULDBLOCK
 #define TelnetPort 23
#else
 #define TelnetPort "23"
#endif

enum{ TS_DATA, TS_IAC, TS_WILL, TS_WONT, TS_DO, TS_DONT, TS_CR, TS_SB, TS_SE };
gchar options[BUFFER_SIZE];              // The combined options
guchar netobuf [BUFFER_SIZE];
guchar *posOBuf;
gint net;
gint inputTag=0;
gboolean echo = TRUE;


void telnetText(char *text){
	static guchar tbuffer[BUFFER_SIZE];
	static gboolean vtSeq=FALSE;
	static gchar vtBuf[16];
	static gint  vtPos=0;
	gint pos=0;
	gint pos2=4;

	if ((gboolean) MW_GET("TELNET_SILENT")) return;

	// interne Meldung ausgeben.
	if (text[pos]=='<' && text[pos+1]=='c' && text[pos+3]=='>'){
		insertText(MW_GET("TELNET_VIEW"), txtIn(text), TRUE);
		return;
	}

	// externe Meldung ausgeben.
	tbuffer[0]='<'; tbuffer[1]='c';tbuffer[2]='0';tbuffer[3]='>';
	while(text[pos]){
		if (text[pos]==27){
			vtSeq=TRUE;
			tbuffer[pos2]=0;	
			pos2=4;
			insertText(MW_GET("TELNET_VIEW"), txtIn(tbuffer), TRUE);
		}
		if (vtSeq==TRUE){
			if (text[pos]==109){
				vtSeq=FALSE;
				vtPos=0;
				tbuffer[2]='0';
				if (!strncmp(vtBuf, "133", 3)) tbuffer[2]='1';
				if (!strncmp(vtBuf, "134", 3)) tbuffer[2]='2';
				if (!strncmp(vtBuf, "135", 3)) tbuffer[2]='3';
				if (!strncmp(vtBuf, "136", 3)) tbuffer[2]='4';
				if (!strncmp(vtBuf, "137", 3)) tbuffer[2]='5';
			}
			if ((text[pos]>='0') && (text[pos]<='9')) vtBuf[vtPos++]=text[pos];
			pos++;
		}
		else tbuffer[pos2++] = text[pos++];
	}
	tbuffer[pos2]=0;
	insertText(MW_GET("TELNET_VIEW"), txtIn(tbuffer), TRUE);
}


void readNet(gpointer data, gint source, GdkInputCondition condition){
	/// !!!! tlogin muss  beim telnet Init auf false gesetzt werden.
	static guchar netInput[BUFFER_SIZE];
	static guchar ttyOut  [BUFFER_SIZE];
	gint c;
	ttyOut[0]=0;

	c = recv(net, netInput, BUFFER_SIZE, 0);
	netInput[c]=0;
	telrcv(netInput, &ttyOut[0]);
	netflush();
	telnetText(ttyOut);

	// login - procedure.
	if(t_login == FALSE){
		if(strFind(ttyOut, STR_USER)){
			sprintf(ttyOut, "%s\n", gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_USER"))));
			send(net, ttyOut, strlen(ttyOut), 0);
			telnetText(ttyOut);
		}
		if(strFind(ttyOut, STR_PASS)){
			int pos=0;
			sprintf(ttyOut, "%s\n", gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_PASS"))));
			send(net, ttyOut, strlen(ttyOut), 0);
			while(ttyOut[pos]!='\n') ttyOut[pos++]='*';
			telnetText(ttyOut);
		}
		if(strFind(ttyOut, STR_LOGIN)) t_login=TRUE;
	}
}

void initTelnet(gboolean silent){
	#ifndef WIN32
	struct addrinfo hints, *res;
	#else
	struct sockaddr_in sa;
	WSADATA info;
	if (WSAStartup(MAKEWORD(1,1), &info)) telnetText("Winsock error");
	#endif
	inputTag=0;
	posOBuf = netobuf;
	netobuf[0]=0;
	errno = 0;

	t_login = FALSE;
	MW_SET("TELNET_SILENT", silent);

  #ifndef WIN32
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	// look for host and connect
	switch(getaddrinfo(gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_IP"))),TelnetPort, &hints, &res)){
		case EAI_FAMILY:
			telnetText("<c1>The requested address family is \nnot  supported  at all\n");
			return;
		case EAI_SOCKTYPE:
			telnetText("<c1>The requested socket type is not supported at all\n");
			return;
		case EAI_BADFLAGS:
			telnetText("<c1>ai_flags contains invalid flags.\n");
			return;
		case EAI_NONAME:
			telnetText("<c1>The node or service is not known.\n");
			return;
		case EAI_SERVICE:
			telnetText("<c1>The requested service is not \navailable for the requested socket type.\n");
			return;
		case EAI_ADDRFAMILY:
			telnetText("<c1>The specified network host does not have any net­"
				"\nwork addresses in the requested address family.\n");
			return;
		case EAI_NODATA:
			telnetText("<c1>The specified network host exists, but does not"
				"\nhave any network addresses defined.\n");
			return;
		case EAI_MEMORY:
			telnetText("<c1>Out of memory.\n");
			return;
		case EAI_FAIL:
			telnetText("<c1>Name server returned a permanent failure indication.\n");
			return;
		case EAI_AGAIN:
			telnetText("<c1>Name server returned a temporary failure indication.\nTry again later.\n");
			return;
		case EAI_SYSTEM:
			telnetText("<c1>Other system error, check errno for details.\n");
			return;
	}
	net = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(net < 0){
		telnetText("<c1>Couldn't create socket\n");
		return;
	}
	if(!bind(net, res->ai_addr, res->ai_addrlen)){
		telnetText("<c1>Bind fehler\n");
		close(net);
		return;
	}
	if(connect(net, res->ai_addr, res->ai_addrlen)){
		telnetText("<c1>Couldn't find host\n");
		close(net);
		return;
	}
	freeaddrinfo(res);
	#else // WIN32
	sa.sin_port = htons(TelnetPort);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(gtk_entry_get_text(GTK_ENTRY(MW_GET("OPT_NET_IP"))));
	net = socket(sa.sin_family, SOCK_STREAM, 0);

	if(!net){
		telnetText("<c1>Socket Error\n");
		return;
	}

	if((connect(net, (struct sockaddr *) &sa, sizeof(sa))) <0){
		close(net);
		telnetText("<c1>** Couldn't connect to host\n");
		return;
	}
	#endif

	if (net<0){
		telnetText("<c1>Connect Error\n");
		return;
	}
	memset((char *)options, 0, sizeof options);
	telnetConnected = 1;
	inputTag = gdk_input_add(net, GDK_INPUT_READ, readNet,NULL);
}

void quitTelnet(){
	telnetConnected=0;
	if (inputTag) gdk_input_remove(inputTag);
	inputTag=0;
	close(net);
	gtk_widget_destroy(MW_GET("TELNET_DIALOG"));
	MW_SET("TELNET_DIALOG", NULL);
}


void telrcv(guchar *netInput, guchar *ttyOut){
	static guchar subbuffer    [BUFFER_SIZE];
	static gchar do_dont_resp  [BUFFER_SIZE];
	static gchar will_wont_resp[BUFFER_SIZE];
	static guchar *subpointer = subbuffer; // sb_clear.
	static gint telrcv_state = TS_DATA;
	guchar c;
	gint pos = 0;
	gint len = strlen(netInput);

	while (len--){
		c= netInput[pos++];
		switch (telrcv_state){
			case TS_CR:
				telrcv_state = TS_DATA;
				if  (c == '\0')  break;
			case TS_DATA:
				if (c == IAC) {
				telrcv_state = TS_IAC;
				break;
				}
				if (c != '\r') *ttyOut++ = c;
				continue;
			case TS_IAC:
				switch (c) {
					case WILL:
						telrcv_state = TS_WILL;
						continue;
					case WONT:
						telrcv_state = TS_WONT;
						continue;
					case DO:
						telrcv_state = TS_DO;
						continue;
					case DONT:
						telrcv_state = TS_DONT;
						continue;
					case SB:
						subpointer = subbuffer; // sb clear.
						telrcv_state = TS_SB;
						continue;
				}
				telrcv_state = TS_DATA;
				continue;
			case TS_WILL:
				// will option
				if (do_dont_resp[c]){
					--do_dont_resp[c];
					if (do_dont_resp[c] && (options[c]&MY_STATE_DO)) --do_dont_resp[c];
				}
				if ((do_dont_resp[c] == 0) &&  (! (options[c]&MY_WANT_STATE_DO)))	{
					do_dont_resp[c]++;
					*posOBuf++ =  IAC;
					*posOBuf++ = DONT;
					*posOBuf++ =  c;
				}
				options[c] |= MY_STATE_DO;
				telrcv_state = TS_DATA;
				continue;
			case TS_WONT:
				// wont option
				if (do_dont_resp[c]){
					--do_dont_resp[c];
					if (do_dont_resp[c] &&  (!(options[c]&MY_STATE_DO) )) --do_dont_resp[c];
				}
				if ((do_dont_resp[c] == 0) &&  (options[c]&MY_WANT_STATE_DO)){
					options[c] &= ~MY_WANT_STATE_DO;
					if (options[c]&MY_STATE_DO){
						*posOBuf++ = IAC;
						*posOBuf++ = DONT;
						*posOBuf++ = c;
					}
				}
				options[c] &= ~MY_STATE_DO;
				telrcv_state = TS_DATA;
				continue;
			case TS_DO: // do option
				if (will_wont_resp[c]){
					--will_wont_resp[c];
					if (will_wont_resp[c] &&  (options[c]&MY_STATE_WILL)) --will_wont_resp[c];
				}
				if (will_wont_resp[c] == 0)	{
					if (!(options[c]&MY_WANT_STATE_WILL)){
						will_wont_resp[c]++;
						*posOBuf++ =  IAC;
						*posOBuf++ = WONT;
						*posOBuf++ = c;
					}
				}
				options[c] |= MY_STATE_WILL;
				telrcv_state = TS_DATA;
				continue;
			case TS_DONT:
				// dont option
				if (will_wont_resp[c]){
					--will_wont_resp[c];
					if (will_wont_resp[c] &&  (! (options[c]&MY_STATE_WILL))) --will_wont_resp[c];
				}
				if ((will_wont_resp[c] == 0) &&  (options[c]&MY_WANT_STATE_WILL)){
					options[c] &= ~MY_WANT_STATE_WILL;
					if ( (options[c]&MY_STATE_WILL)){
						*posOBuf++ =  IAC;
						*posOBuf++ = WONT;
						*posOBuf++ = c;
					}
				}
				options[c] &= ~MY_STATE_WILL;
				telrcv_state = TS_DATA;
				continue;
			case TS_SB:
				if (c == IAC) telrcv_state = TS_SE;
				else if (subpointer < (subbuffer+sizeof subbuffer))  *subpointer++ = c;
				continue;
			case TS_SE:
				if (subpointer < (subbuffer+sizeof subbuffer))  *subpointer++ = IAC;
				if (subpointer < (subbuffer+sizeof subbuffer))  *subpointer++ = SE;
				subpointer = subbuffer;
				switch (*subpointer++){
					case TELOPT_TTYPE:
						if (!(options[TELOPT_TTYPE]&MY_WANT_STATE_WILL)) return;
						sprintf(posOBuf, "%c%c%c%c%s%c%c",IAC,SB, TELOPT_TTYPE, TELQUAL_IS, "UNKNOWN", IAC, SE);
						posOBuf+=strlen("UNKNOWN")+4+2;
						break;
					case TELOPT_TSPEED:
						if (!(options[TELOPT_TSPEED]&MY_WANT_STATE_WILL)) return;
						sprintf(posOBuf, "%c%c%c%c%s%c%c",IAC,SB,TELOPT_TSPEED,TELQUAL_IS,"38400,38400",IAC,SE);
						posOBuf+=strlen("38400,38400")+4+2;
						break;
					case TELOPT_NEW_ENVIRON:
						sprintf(posOBuf, "%c%c%c%c%s%c%c",IAC,SB,TELOPT_NEW_ENVIRON, TELQUAL_IS, " ", IAC, SE);
						posOBuf += strlen(" ")+4+2;
					break;
				}
				telrcv_state = TS_DATA;
		}
	}
	netInput[0]=0;
	*ttyOut = 0;
}

void netflush(void){
	if ((send(net, (gchar*)netobuf, posOBuf - netobuf, 0) < 0)
 	&& errno != ENOBUFS && errno != EWOULDBLOCK){
		telnetText("<c1>\nLost connection to host\nPlease close the telnet-window and connect again\n");
		if (inputTag) gdk_input_remove(inputTag);
		inputTag=0;
		close(net);
		return;
	}
	posOBuf= netobuf; // empty Buffer.
}

void sendTelnet(gchar *command){
	if(!telnetConnected){
		telnetText("<c1>Not connected!\n");
		return;
	}
	if ((!strncmp(command,"exit",4)) || (!strncmp(command,"logout",6))){
		quitTelnet();
		return;
	}
	echo = FALSE;
	if ((send(net, command ,strlen(command), 0) < 0) && errno != ENOBUFS && errno != EWOULDBLOCK){
		telnetText("<c1>\nLost connection to host\nPlease close the telnet-window and connect again\n");
		if (inputTag) gdk_input_remove(inputTag);
		inputTag=0;
		close(net);
	}
}


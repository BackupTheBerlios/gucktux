CC      = gcc
CFLAGS  = `pkg-config --cflags gtk+-2.0 gthread-2.0`
LDFLAGS = `pkg-config --libs gtk+-2.0 gthread-2.0`
OBJECTS = main.o dialog.o enigma.o ftp.o neutrino.o lcars.o listen.o telnet.o tools.o mainwindow.o
TARGET  = gucktux.linux

all:  $(TARGET)

clean:
	rm *.o
	rm gucktux.linux

$(TARGET):$(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o$(TARGET) $(OBJECTS)


 
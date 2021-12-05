CFLAGS = -P -tW -c -w-8004 -w-8027 -w-8057 -w-8012
CC = bcc32 $(CFLAGS)
LFLAGS = -tW
LINK = bcc32 -e$(EXEC) $(LFLAGS)
RCC = brc32

EXEC = EyesOn.exe
OBJS = CalTime.obj DialogBox.obj EyesOn.obj OpeString.obj Variable.obj
RESS = EyesOn.rc

$(EXEC) : $(OBJS) $(RESS)
	$(LINK) $(OBJS)
	$(RCC) $(RESS) $(EXEC)

CalTime.obj : CalTime.cpp CalTime.h Variable.h
	$(CC) CalTime.cpp

DialogBox.obj : DialogBox.cpp resource.h EyesOn.h DialogBox.h
	$(CC) DialogBox.cpp

EyesOn.obj : EyesOn.cpp resource.h EyesOn.h CalTime.h Variable.h OpeString.h DialogBox.h
	$(CC) EyesOn.cpp

OpeString.obj : OpeString.cpp OpeString.h
	$(CC) OpeString.cpp

Variable.obj : Variable.cpp Variable.h
	$(CC) Variable.cpp


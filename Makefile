# Makefile for DFS
#
# ANKITA PAWAR & DEEPTI RAGHA
# 
CC=gcc -g -Wall `pkg-config fuse --cflags --libs`
SRC=dfsserver.c
SRC1=dfsclient.c
SRC2=dfsslave.c
CALL= dfsserver dfsclient dfsslave
TARSRC= dfsserver.c dfsclient.c dfsslave.c

all: dfsserver dfsclient dfsslave

dfsserver:$(SRC) 
	$(CC) $(SRC) -o dfsserver
	

dfsclient: $(SRC1)
	$(CC) $(SRC1) -o dfsclient

dfsslave:$(SRC2) 
	$(CC) $(SRC2) -o dfsslave
	
tar:
	tar czvf dfs.tar.gz $(TARSRC) Makefile README Enhancement

clean:
	\rm $(CALL)

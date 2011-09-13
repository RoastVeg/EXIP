# Common module build

COMMON_OBJ = $(BIN_DIR)/p_ASCII_stringManipulate.o $(BIN_DIR)/procTypes.o \
			 $(BIN_DIR)/contentHandler.o $(BIN_DIR)/memManagement.o \
			 $(BIN_DIR)/hashtable.o $(BIN_DIR)/dynamicArray.o

$(BIN_DIR)/p_ASCII_stringManipulate.o : $(COMMON_SRC)/src/p_ASCII_stringManipulate.c $(PUBLIC_INCLUDE_DIR)/stringManipulate.h $(PUBLIC_INCLUDE_DIR)/procTypes.h $(PUBLIC_INCLUDE_DIR)/errorHandle.h
		$(CC) -c $(CFLAGS) $(COMMON_SRC)/src/p_ASCII_stringManipulate.c -o $(BIN_DIR)/p_ASCII_stringManipulate.o
	
$(BIN_DIR)/procTypes.o : $(COMMON_SRC)/src/procTypes.c $(PUBLIC_INCLUDE_DIR)/procTypes.h $(PUBLIC_INCLUDE_DIR)/errorHandle.h
		$(CC) -c $(CFLAGS) $(COMMON_SRC)/src/procTypes.c -o $(BIN_DIR)/procTypes.o
		
$(BIN_DIR)/contentHandler.o : $(COMMON_SRC)/src/contentHandler.c $(PUBLIC_INCLUDE_DIR)/procTypes.h $(PUBLIC_INCLUDE_DIR)/errorHandle.h $(PUBLIC_INCLUDE_DIR)/contentHandler.h
		$(CC) -c $(CFLAGS) $(COMMON_SRC)/src/contentHandler.c -o $(BIN_DIR)/contentHandler.o
		
$(BIN_DIR)/memManagement.o : $(PUBLIC_INCLUDE_DIR)/procTypes.h $(PUBLIC_INCLUDE_DIR)/errorHandle.h $(COMMON_SRC)/include/memManagement.h $(COMMON_SRC)/src/memManagement.c
		$(CC) -c $(CFLAGS) $(COMMON_SRC)/src/memManagement.c -o $(BIN_DIR)/memManagement.o
		
$(BIN_DIR)/hashtable.o : $(COMMON_SRC)/include/hashtable.h $(COMMON_SRC)/include/hashtable_private.h $(COMMON_SRC)/src/hashtable.c
		$(CC) -c $(CFLAGS) $(COMMON_SRC)/src/hashtable.c -o $(BIN_DIR)/hashtable.o
		
$(BIN_DIR)/dynamicArray.o: $(COMMON_SRC)/include/dynamicArray.h $(COMMON_SRC)/src/dynamicArray.c
		$(CC) -c $(CFLAGS) $(COMMON_SRC)/src/dynamicArray.c -o $(BIN_DIR)/dynamicArray.o	

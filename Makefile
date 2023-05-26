MAIN_FILE=CODIGO COMPLETO 1.10
CC=ccsc
SRC_DIR =src
OUT_DIR=build
CFLAGS=+DF +A I=$(INC_DIR) I+=$(INC_DIR2) out=$(OUT_DIR) -L -J -M
INC_DIR="C:/Program Files (x86)/PICC/Devices"
INC_DIR2="C:/Program Files (x86)/PICC/Drivers"


all:
	$(CC) $(CFLAGS) '$(SRC_DIR)/$(MAIN_FILE).c'
	more '$(OUT_DIR)/$(MAIN_FILE).err'
	more '$(OUT_DIR)/$(MAIN_FILE).STA'

clean: $(OUT_DIR)
	del /S /Q $(OUT_DIR)\*

setup:
	mkdir $(SRC_DIR)
	mkdir $(OUT_DIR)
	echo #include ^<16F874.h^> > "$(SRC_DIR)/$(MAIN_FILE).c"
	echo. >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo #ifndef _CLION_IDE_ >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo #fuses HS, NOWDT, NOPROTECT, NOLVP >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo #use delay(clock= 20000000) >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo #endif >> "$(SRC_DIR)/$(MAIN_FILE).c
	echo. >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo int main^(^) { >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo     return 0; >> "$(SRC_DIR)/$(MAIN_FILE).c"
	echo ^} >> "$(SRC_DIR)/$(MAIN_FILE).c"
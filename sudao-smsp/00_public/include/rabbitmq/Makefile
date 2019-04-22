GCC=gcc
TARGET = librabbitmq
LIB = librabbitmq.a
CPPFLAGS = -c
CFLAGS = -g -O3 -Wall -DLINUX2 -D_REENTRANT -DOS_LINUX -DHAVE_POLL
AR=ar
OBJ_DIR = ./${TARGET}
INC = ./
SRC = $(wildcard $(INC)/*.c)
OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRC)))
$(LIB):$(OBJ)
	$(AR) r $@ $(OBJ)

$(OBJ_DIR)/%.o:$(INC)/%.c
	$(GCC) $(CFLAGS) -c  $< -o $@ $(INC)
clean:
	rm -rf $(OBJ_DIR)/*

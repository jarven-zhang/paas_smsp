GCC = g++ -g
INPUT = $(shell echo $(lib) | tr a-z A-Z)
TARGET = lib$(INPUT).so

SHARED = -shared
FPIC = -fPIC


### route setting
ROOT_DIR = ../..
PUBLIC_DIR = $(ROOT_DIR)/../00_public
TARGET_DIR = $(ROOT_DIR)/build/smsp_plugin/lib
COMM_DIR = $(ROOT_DIR)/public/common
MODELS_DIR = $(ROOT_DIR)/public/models
XML_DIR = $(ROOT_DIR)/public/xml
INC_DIR = $(PUBLIC_DIR)/include
LIB_DIR = $(PUBLIC_DIR)/libraries
OBJ_DIR = ./bin
PLUGIN_DIR = ./${INPUT}


### create a "bin" directory
$(shell mkdir -p ${OBJ_DIR})
RM = rm -rf


### flags setting
CFLAGS = -g -O0 -Wall -DLINUX2 -D_REENTRANT -DOS_LINUX


### include setting
COMM_INC = -I$(COMM_DIR) -I$(MODELS_DIR)
XML_INC = -I$(XML_DIR)
LIB_INC = -I$(INC_DIR)
PLUGIN_INC = -I$(PLUGIN_DIR)
INC = $(COMM_INC) $(LIB_INC) $(PLUGIN_INC) $(XML_INC)


### libraries setting
SMSP_LIB = -ljson -lssl
LIB_PATH = $(LIB_DIR)/debug

LIB = -L$(LIB_PATH) $(SMSP_LIB)


### srcource code setting
PLUGIN_SRC = $(wildcard $(PLUGIN_DIR)/*.cpp)
COMM_SRC = $(shell ls $(COMM_DIR)/*.cpp| egrep "(SmsContext.|Channellib.|StateReport|UpstreamSms|UTFString)")
MODELS_SRC = $(shell ls $(MODELS_DIR)/*.cpp| egrep "(Channel.h|Channel.cpp)")
XML_SRC = $(wildcard $(XML_DIR)/*.cpp)


SRC = $(COMM_SRC) $(MODELS_SRC) $(PLUGIN_SRC) $(XML_SRC)


### object setting
OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRC))) 


### target setting
$(OBJ_DIR)/$(TARGET):$(OBJ)
	$(GCC) $(OBJ) $(SHARED) -o $@ $(LIB)
	mv $(OBJ_DIR)/$(TARGET) $(TARGET_DIR)/$(TARGET)


### generated object file
$(OBJ_DIR)/%.o:$(COMM_DIR)/%.cpp
	$(GCC) $(FPIC) -c  $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(MODELS_DIR)/%.cpp
	$(GCC) $(FPIC) -c  $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(XML_DIR)/%.cpp
	$(GCC) $(FPIC) -c  $< -o $@ $(INC)

$(OBJ_DIR)/%.o:$(PLUGIN_DIR)/%.cpp
	$(GCC) $(FPIC) -c  $< -o $@ $(INC)

### remove object
.PHONY : clean
clean :
	$(RM) $(OBJ_DIR)


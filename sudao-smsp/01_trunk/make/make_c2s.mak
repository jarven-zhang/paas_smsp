GCC = g++
TARGET = smsp_c2s


### public lib
CUR_NAME = $(shell basename `cd ..;pwd;`)
PUBLC_DIR = ../../../00_public
ifeq ($(CUR_NAME),01_trunk)
	PUBLC_DIR = ../../00_public
endif

INC_DIR = $(PUBLC_DIR)/include
SURC_DIR = $(PUBLC_DIR)/source
LIB_DIR = $(PUBLC_DIR)/libraries

### route setting
ROOT_DIR = ..
TARGET_DIR = $(ROOT_DIR)/build/${TARGET}/bin
PUB_DIR = $(ROOT_DIR)/public
SMSP_DIR = $(ROOT_DIR)/source/$(TARGET)
OBJ_DIR = ./${TARGET}


### create a "bin" directory
$(shell mkdir -p ${OBJ_DIR})
RM = rm -rf


### flags setting
CFLAGS = -g -O3 -Wall -DLINUX2 -D_REENTRANT -DOS_LINUX -D ${TARGET}


### include setting
SYS_INC = \
	-I/usr/include/mysql
SMSP_INC = \
	-I$(SMSP_DIR) \
	-I$(SMSP_DIR)/thread \
	-I$(SMSP_DIR)/direct 
LIB_INC = \
	-I$(INC_DIR) \
	-I$(INC_DIR)/hiredis \
	-I$(INC_DIR)/rabbitmq \
	-I$(INC_DIR)/opencc
SURC_INC = \
	-I$(SURC_DIR) 
PUB_INC = \
	-I$(PUB_DIR) \
	-I$(PUB_DIR)/base \
	-I$(PUB_DIR)/common \
	-I$(PUB_DIR)/http \
	-I$(PUB_DIR)/models \
	-I$(PUB_DIR)/net \
	-I$(PUB_DIR)/pack \
	-I$(PUB_DIR)/thread \
	-I$(PUB_DIR)/thread/base \
	-I$(PUB_DIR)/thread/ruleloadthread \
	-I$(PUB_DIR)/thread/mqthread \
	-I$(PUB_DIR)/thread/mqthread/mqthread \
	-I$(PUB_DIR)/thread/mqthread/mqthreadpool \
	-I$(PUB_DIR)/xml \
	-I$(PUB_DIR)/mqthread \
	-I$(PUB_DIR)/protocol
INC = $(SYS_INC) $(SMSP_INC) $(LIB_INC) $(SURC_INC) $(PUB_INC)


### libraries setting
SYS_LIB = -lpthread -luuid -lmysqlclient -lcares -L/usr/lib64/mysql -lcurl -L/usr/local/lib
SMSP_LIB = -lhiredis -ljson -lssl -lboost_regex-gcc-d-1_45 -lrabbitmq -lrt -lopencc -lboost_date_time -lboost_thread
LIB_PATH = $(LIB_DIR)/debug
LIB_CHANNEL = -Wl,--rpath,lib/:../lib/

LIB = $(LIB_CHANNEL) $(SYS_LIB) $(SMSP_LIB) -L$(LIB_PATH)


### srcource code setting
PUB_SRC = \
	$(wildcard $(PUB_DIR)/base/*.cpp) \
	$(wildcard $(PUB_DIR)/common/*.cpp) \
	$(wildcard $(PUB_DIR)/http/*.cpp) \
	$(wildcard $(PUB_DIR)/models/*.cpp) \
	$(wildcard $(PUB_DIR)/net/*.cpp) \
	$(wildcard $(PUB_DIR)/pack/*.cpp) \
	$(wildcard $(PUB_DIR)/thread/*.cpp)\
	$(wildcard $(PUB_DIR)/thread/base/*.cpp) \
	$(wildcard $(PUB_DIR)/thread/ruleloadthread/*.cpp) \
	$(wildcard $(PUB_DIR)/thread/mqthread/*.cpp) \
	$(wildcard $(PUB_DIR)/thread/mqthread/mqthread/*.cpp) \
	$(wildcard $(PUB_DIR)/thread/mqthread/mqthreadpool/*.cpp) \
	$(wildcard $(PUB_DIR)/xml/*.cpp) \
	$(wildcard $(PUB_DIR)/mqthread/*.cpp) \
	$(wildcard $(PUB_DIR)/protocol/*.cpp)
SMSP_SRC = \
	$(wildcard $(SMSP_DIR)/*.cpp) \
	$(wildcard $(SMSP_DIR)/thread/*.cpp) \
	$(wildcard $(SMSP_DIR)/direct/*.cpp)
SRC = $(PUB_SRC) $(SMSP_SRC)


### object setting
OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRC)))


### target setting
$(TARGET_DIR)/$(TARGET):$(OBJ)
	$(GCC) $(OBJ)  -o $@ $(LIB)


### generated object file

$(OBJ_DIR)/%.o:$(PUB_DIR)/base/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/common/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/http/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/models/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/net/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/pack/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/thread/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/thread/base/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/thread/ruleloadthread/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/thread/mqthread/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/thread/mqthread/mqthread/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/thread/mqthread/mqthreadpool/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/xml/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/mqthread/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(PUB_DIR)/protocol/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)

$(OBJ_DIR)/%.o:$(SMSP_DIR)/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(SMSP_DIR)/thread/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)
$(OBJ_DIR)/%.o:$(SMSP_DIR)/direct/%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@ $(INC)

### remove object
.PHONY : clean
clean :
	$(RM) $(OBJ_DIR)



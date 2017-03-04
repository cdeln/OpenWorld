#CC = gcc
#GXX = g++
PP_FLAGS = -DGL_GLEXT_PROTOTYPES
CC_FLAGS = -Wall
CXX_FLAGS = -w -std=c++11 

COMMON_DIR=common
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	PLATFORM = Mac
else
	PLATFORM = Linux
endif	

ifeq (${PLATFORM}, Mac)
	M_SRC = $(COMMON_DIR)/$(PLATFORM)/MicroGlut.m
	CC_SRC = $(COMMON_DIR)/GL_utilities.c $(COMMON_DIR)/VectorUtils3.c $(COMMON_DIR)/loadobj.c $(COMMON_DIR)/LoadTGA.c
	LINK_FLAGS = -framework OpenGL -framework Cocoa -lm -lopencv_core -lopencv_imgproc -lopencv_highgui
else
	CC_SRC = $(COMMON_DIR)/$(PLATFORM)/MicroGlut.c $(COMMON_DIR)/GL_utilities.c $(COMMON_DIR)/VectorUtils3.c $(COMMON_DIR)/loadobj.c $(COMMON_DIR)/LoadTGA.c
	LINK_FLAGS = -lXt -lX11 -lGL -lm -lopencv_core -lopencv_imgproc -lopencv_highgui
	INCLUDE_EIGEN = $(shell pkg-config --cflags eigen3)
endif

OPENCV_VERSION = $(shell pkg-config --modversion opencv)
OPENCV_VERSION_MAJOR = $(shell echo $(OPENCV_VERSION) | cut -f1 -d.)

SRC_EXE_DIR = src
SRC_LIB_DIR = src/lib

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
EXE_DIR = $(BUILD_DIR)/exe
DATA_DIR = data
MODEL_DIR = $(DATA_DIR)/models
TEX_DIR = $(DATA_DIR)/textures
REC_DIR = $(DATA_DIR)/record

CC_SRC += $(wildcard $(SRC_LIB_DIR)/*.c)
CXX_SRC = $(wildcard $(SRC_LIB_DIR)/*.cpp)
#CC_EXE_SRC = $(wildcard $(SRC_EXE_DIR)/*.c)
EXE_SRC = $(wildcard $(SRC_EXE_DIR)/*.cpp)
#EXE_SRC = src/open_world_final.cpp src/system_test.cpp

M_OBJ = $(M_SRC:.m=.o)
CC_OBJ = $(CC_SRC:.c=.o)
CXX_OBJ = $(CXX_SRC:.cpp=.o)
#CC_EXE_OBJ = $(CC_EXE_SRC:.c=.exe)
EXE_OBJ = $(EXE_SRC:.cpp=.exe)

EXE_FINAL_NAME = open_world_final

# External directories
EXT_DIR = ext
GLM_DIR = $(EXT_DIR)/glm
FFTW_NAME = fftw-3.3.6-pl2
FFTW_DIR = $(EXT_DIR)/$(FFTW_NAME)
#OPENCV_DIR = ext/opencv

# External lib flags
GLM_FLAGS = -DGLM_ENABLE_EXPERIMENTAL
EXT_FLAGS = $(GLM_FLAGS)

# Headers
GLM_INCL_DIR = $(GLM_DIR)
FFTW_INCL_DIR = $(FFTW_DIR)/api
CC_INCLUDE = -I$(COMMON_DIR) -I$(COMMON_DIR)/$(PLATFORM) 
CXX_INCLUDE = -I$(SRC_LIB_DIR) 
EXT_INCLUDE = -I$(GLM_INCL_DIR) -I$(FFTW_INCL_DIR)

# Libraries
FFTW_LIB_DIR=$(FFTW_DIR)/.libs
FFTW_LIB=$(FFTW_LIB_DIR)/libfftw3.a
#OPENCV_LIB_DIR = $(OPENCV_DIR)/build/lib
#OPENCV_LIB = $(wildcard $(OPENCV_LIB_DIR)/*.dylib) #$(wildcard $(OPENCV_LIB_DIR)/*.dylib)

FOUND_FFTW = $(shell pkg-config --exists fftw3; echo $$?)
ifeq ($(FOUND_FFTW),1)
	LIBS = $(FFTW_LIB)
else
	LINK_FLAGS += -lfftw3
endif

all: MOVE #$(EXE_OBJ)

TEXTURES:
	if [ ! -d $(TEX_DIR)/clouds ]; then cd $(DATA_DIR); tar -zxvf textures.tar.gz; fi

$(FFTW_DIR):
	@echo "FFTW3 NOT FOUND ON FILE SYSTEM, DOWNLOADING FROM WEBSITE"
	cd $(EXT_DIR); curl http://fftw.org/$(FFTW_NAME).tar.gz -o $(FFTW_NAME).tar.gz; gzip -d $(FFTW_NAME).tar.gz; tar -xvf $(FFTW_NAME).tar; rm $(FFTW_NAME).tar;

$(FFTW_LIB): $(FFTW_DIR)
	#$(eval FFTW_LIB_CHECK = $(shell pkg-config --exists fftw3; echo $$?))
	#@if [ $(FFTW_LIB_CHECK) = 1 ]; then echo "FFTW3 NOT FOUND ON SYSTEM, BUILDING FROM SOURCE"; cd $(FFTW_DIR); ./bootstrap.sh; make; else echo "FFTW3 LIB FOUND ON SYSTEM, USING EXISTING INSTALLATION"; fi 
	#cd $(FFTW_DIR); ./configure; make;
	@if [ ! -f $(FFTW_LIB) ]; then echo "FFTW3 NOT BUILD YET, BUILDING"; cd $(FFTW_DIR); ./configure; make; fi 

$(M_OBJ) : $(M_SRC) DIRS
	$(eval OBJ_NAME=$(notdir $@))
	$(eval SRC_NAME=$(patsubst %.o, %.m, $@))
	$(CC) -c $(CC_FLAGS) $(CC_INCLUDE) $(PP_FLAGS) -o $(OBJ_DIR)/$(OBJ_NAME) $(SRC_NAME) $(LINK_FLAGS)

$(CC_OBJ): $(CC_SRC) DIRS
	$(eval OBJ_NAME=$(notdir $@))
	$(eval SRC_NAME=$(patsubst %.o, %.c, $@))
	$(CC) -c $(CC_FLAGS) $(CC_INCLUDE) $(PP_FLAGS) -o $(OBJ_DIR)/$(OBJ_NAME) $(SRC_NAME) $(LINK_FLAGS)

$(CXX_OBJ): $(CXX_SRC) $(LIBS) DIRS 
	$(eval OBJ_NAME=$(notdir $@))
	$(eval SRC_NAME=$(patsubst %.o, %.cpp, $@))
	$(CXX) $(CXX_FLAGS) $(EXT_FLAGS) -c $(CC_INCLUDE) $(EXT_INCLUDE) $(PP_FLAGS) -o $(OBJ_DIR)/$(OBJ_NAME) $(SRC_NAME) $(LINK_FLAGS)

$(EXE_OBJ): $(M_OBJ) $(CC_OBJ) $(CXX_OBJ) $(EXE_SRC) $(LIBS) TEXTURES DIRS
	$(eval OBJ_NAME=$(notdir $@))
	$(eval SRC_NAME=$(patsubst %.exe, %.cpp, $@))
	$(CXX) $(CXX_FLAGS) $(EXT_FLAGS) $(CC_INCLUDE) $(CXX_INCLUDE) $(EXT_INCLUDE) $(PP_FLAGS) -o $(EXE_DIR)/$(OBJ_NAME) $(wildcard $(OBJ_DIR)/*.o) $(SRC_NAME) $(LINK_FLAGS) $(LIBS)

MOVE: $(EXE_OBJ)
	cp $(EXE_DIR)/$(EXE_FINAL_NAME).exe ./run

DIRS:
	$(eval LINK_FLAGS += $(shell if [ $(OPENCV_VERSION_MAJOR) > "2" ]; then echo -lopencv_imgcodecs; fi)) 
	mkdir -p $(BUILD_DIR)
	mkdir -p $(EXE_DIR)
	mkdir -p $(OBJ_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/

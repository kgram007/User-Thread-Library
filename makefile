##################  Assignment #3  #####################
#
#	File Name:	 makefile
#	Author:		 Ramsundar K G
#	Date:		 
#
#	Description: Make File
#
#######################################################


# Output File Names
EXE=libuthread.out

# Folders Names
BIN_DIR=bin
OBJ_DIR=_Obj

# Folder Paths
MAKE_PATH = $(shell pwd)
PROJ_PATH = $(MAKE_PATH)
BIN_PATH = $(PROJ_PATH:=/$(BIN_DIR))
INCLUDE_PATH = $(PROJ_PATH)
SOURCE_PATH = $(PROJ_PATH)
OBJ_PATH = $(SOURCE_PATH:=/$(OBJ_DIR))




TEST_DIR=test
TRACE=trace.dat
TEST_PATH = $(PROJ_PATH:=/$(TEST_DIR))




# Libraries
LIBS +=-lrt

# Includes Path
CFLAGS=-I$(INCLUDE_PATH)

# Object Files
OBJ_FILES +=  Uthread_Lib.o
OBJ_FILES +=  Scheduler.o

# Objectt File Paths
OBJ = $(patsubst %,$(OBJ_PATH)/%,$(OBJ_FILES))


################# Make Commands #################

$(OBJ_PATH)/%.o: %.c
	@mkdir -p $(OBJ_PATH)
	@echo "Compiling C File.... " $^
	@echo "Creating Obj File... " $(^:.c=.o)
	gcc -c -o $@ $< $(CFLAGS)

lib_uthread: $(OBJ)
	@echo "Building uthread Lib.... "
	ar rc libuthread.a $(OBJ)
	ranlib libuthread.a

#code: $(OBJ)
#	@gcc -c $^ $(LIBS) $(CFLAGS)

mkdir_obj:
	@mkdir -p $(OBJ_PATH)



clean:
	rm -rf $(OBJ_PATH)/*.o ./libuthread.a

##################################################

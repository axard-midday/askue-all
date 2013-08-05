# 
# Сборка для libaskue 2.0.0
#

project_name := libaskue
majorVersion := 2
minorVersion := 0.0
fullVersion := $(majorVersion).$(minorVersion)
source_dirs := ../src
dest_dir := ../bin

source_files := $(wildcard $(addsuffix /*.c, $(source_dirs)))

object_files := $(notdir $(source_files))
object_files := $(object_files:.c=.o)

link_flags := -lsqlite3 -lconfig -shared 
compile_flags := -std=c99 -fPIC
soname_flag := -Wl,-soname,$@.so.$(majorVersion)

CC := gcc-3.4

output := $(addprefix $(dest_dir), $(project_name) )
output := $(addsuffix .so., $(project_name))
output := $(addsuffix $(fullVersion), $(output))

export_libpath := $(shell export LIBRARY_PATH=/usr/lib/i386-linux-gnu:$$LIBRARY_PATH)

all: Init_Library_Path $(project_name) Clean_Object_Files Move_BinDir

Move_BinDir:
	mkdir $(dest_dir)
	mv $(output) $(addsuffix $(output), $(addsuffix /, $(dest_dir)))

Clean_Object_Files:
	rm *.o

Init_Library_Path:
	$(export_libpath)

$(project_name): $(object_files)
	$(CC) $^ -o $(output) $(soname_flag) -pipe $(link_flags)

%.o:%.c 
	$(CC) $^ -c $(compile_flags) $(addprefix -I, $(source_dirs)) -pipe 
	
VPATH := $(source_dirs)

.PHONY: all Init_Library_Path Clean_Object_Files Move_BinDir

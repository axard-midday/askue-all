# 
# Сборка для libaskue 0.0.1
#

project_name := libaskue
majorVersion := 2
minorVersion := 0.0
fullVersion := $(majorVersion).$(minorVersion)
source_dirs := .

source_dirs := $(addprefix ../src/, $(source_dirs))
source_files := $(wildcard $(addsuffix /*.c, $(source_dirs)))
	
object_files := $(notdir $(source_files))
object_files := $(object_files:.c=.o)

link_flags := -lsqlite3 -lconfig -shared
compile_flags := -std=c99 -fPIC
soname_flag := -Wl,-soname,$@.so.$(majorVersion)

output := $(addsuffix .so., $(project_name))
output := $(addsuffix $(fullVersion), $(output))

$(project_name): $(object_files)
	gcc $^ -o $(output) $(soname_flag) -pipe $(link_flags)
	rm *.o
	./mkbin $(output)

%.o:%.c 
	gcc $^ -c $(compile_flags) $(addprefix -I, $(source_dirs)) -pipe 
	
VPATH := $(source_dirs)


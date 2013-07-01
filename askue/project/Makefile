# 
# Сборка для libaskue 0.0.1
#

project_name := askue_launcher
source_dirs := . 

source_dirs := $(addprefix ../../src/, $(source_dirs))
source_files := $(wildcard $(addsuffix /*.c, $(source_dirs)))
	
object_files := $(notdir $(source_files))
object_files := $(object_files:.c=.o)

link_flags := -lconfig -laskue -lsqlite3

$(project_name): $(object_files)
	gcc $^ -o $(project_name) -pipe -L/usr/local/lib/ $(link_flags)
	rm *.o
	strip -s $(project_name)

%.o:%.c
	gcc $^ -c $(compile_flags) -O2 -pipe 2>log
	
VPATH := $(source_dirs)

include $(wildcard *.d)

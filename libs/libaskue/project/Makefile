# 
# Сборка для libaskue 0.0.1
#

project_name := libaskue
source_dirs := . Types Utilites Protocols

source_dirs := $(addprefix ../../src/, $(source_dirs))
source_files := $(wildcard $(addsuffix /*.c, $(source_dirs)))
	
object_files := $(notdir $(source_files))
object_files := $(object_files:.c=.o)

link_flags := -lsqlite3 -lconfig

$(project_name): $(object_files)
	gcc $^ -g2 -shared -o $(addsuffix .so.1.2, $@) -Wl,-soname,$@.so.1 -pipe $(link_flags)
	rm *.o

%.o:%.c 
	gcc $^ -g2 -fPIC -c $(compile_flags) $(addprefix -I, $(source_dirs)) -pipe 2>log
	
#PHONY := install

install:
	find /usr/ -type f -name "libaskue.*" -delete
	find /usr/ -type l -name "libaskue.*" -delete
	find /usr/ -type l -name "libconfig.so.9" -delete
	cp libaskue.so.1.2 /usr/local/lib/libaskue.so.1.2
	ln -s /usr/local/lib/libaskue.so.1.2 /usr/lib/libaskue.so 
	ln -s /usr/local/lib/libaskue.so.1.2 /usr/lib/libaskue.so.1
	ln -s /usr/local/lib/libconfig.so.9.1.3 /usr/lib/libconfig.so.9 
	ln -s /usr/local/lib/libaskue.so.1.2 /usr/local/lib/libaskue.so 
	ln -s /usr/local/lib/libaskue.so.1.2 /usr/local/lib/libaskue.so.1
	cp -r ./libaskue/ /usr/local/include/
	cp ./libaskue.h /usr/local/include/
	
VPATH := $(source_dirs)


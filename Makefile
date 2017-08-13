############################################
# Makefile
# Author: qiyunlong
# Date: 2017年 07月 15日 星期六 13:27:20 CST
############################################

C+ = g++
CC = gcc
CC_FLAG = -O0 -g

HOOK_SRC_FILES := \
	hookMian.cpp \

MAIN_SRC_FILES := \
	mainprocess.cpp \

#set your inc and lib
INC =
LIB = -ldl

.PHONY:hookmain

hookmain:
	$(C+) $(CC_FLAG) $(INC) -o $@ $(HOOK_SRC_FILES) $(LIB)

.PHONY:mainproc
mainproc:
	$(C+) $(CC_FLAG) -o $@ $(MAIN_SRC_FILES)

.PRONY:clean
clean:
	rm -f hookmain mainproc

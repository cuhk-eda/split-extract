LODEPNG_PATH = ./lodepng
LODEPNG_INC = -I$(LODEPNG_PATH)/include
LODEPNG_OBJS = $(LODEPNG_PATH)/src/lodepng.o

MP_OPT = -fopenmp

ifeq ($(mode),asan)
	OPT = -fsanitize=address -O0 -g $(MP_OPT)
	LIBS = -pthread -ldl
	CFLAG =
else
ifeq ($(mode),debug)
	OPT= -O0 -ggdb -DDEBUG $(MP_OPT)
	LIBS = -pthread -ldl
	CFLAG =
else
ifeq ($(mode),profile)
	OPT= -O3 -pg $(MP_OPT)
	LIBS = -pthread -ldl
	CFLAG =
else
ifeq ($(mode),release)
	OPT= -O3 -DNDEBUG $(MP_OPT)
	LIBS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -ldl
	CFLAG = -static
else
ifeq ($(mode),tsan)
	OPT = -fsanitize=thread -O0 -g $(MP_OPT)
	LIBS = -pthread -ldl
	CFLAG =
else
	OPT= -O3 $(MP_OPT)
	LIBS = -pthread -ldl
	CFLAG =
endif
endif
endif
endif
endif

WFLAG    = -Wall -Werror

ifeq ($(origin target),undefined)
	target = bin
endif

ifneq ($(wildcard /home/hcli/usr/local/bin/bison),)
	BISON = /home/hcli/usr/local/bin/bison
else
	BISON = bison
endif

ifneq ($(wildcard /research/fyyoung/hcli/usr/local/bin/flex),)
	FLEX = /research/fyyoung/hcli/usr/local/bin/flex
else
ifneq ($(wildcard /home/hcli/usr/local/bin/flex),)
	FLEX = /home/hcli/usr/local/bin/flex
else
	FLEX = flex
endif
endif

AR    = ar rcs
CC = $(CXX) -std=c++17 $(OPT) $(WFLAG) $(CFLAG) $(LODEPNG_INC) -I.

CC_OBJS = main.o
DB_OBJS = db/db.o db/db_add.o db/db_cell.o db/db_drc.o db/db_geom.o db/db_get.o db/db_image.o db/db_layer.o db/db_net.o db/db_pin.o db/db_td.o db/db_via.o db/db_map.o db/db_place.o
IO_OBJS = io/file_bkshf_db.o io/file_cap.o io/file_lefdef_db.o io/file_liberty.tab.o io/file_liberty.yy.o io/io.o io/utils.o
UT_OBJS = ut/utils.o ut/log.o ut/timer.o
STA_OBJS = sta/sta.o
LIB_OBJS = def58/lib/libdef.a \
           lef58/lib/liblef.a

OBJS = $(CC_OBJS) $(LODEPNG_OBJS) $(UT_OBJS) $(DB_OBJS) $(IO_OBJS) $(STA_OBJS)

BFILE = extract

define copy_build
	mkdir -p ../$(target)
	cp -u $(BFILE) ../$(target)/
endef

.PHONY: all
all: $(BFILE)
	$(call copy_build)


.PHONY: install
install: $(BFILE)
	$(call copy_build)

.PHONY: io
io: $(IO_OBJS)
	$(CC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)
.PHONY: db
db: $(DB_OBJS)
	$(CC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)

$(BFILE): $(OBJS)
	$(CC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)

io/%.tab.o: io/%.y
	$(BISON) -v -p$* -d io/$*.y
	mv $*.tab.c io/$*.tab.c
	mv $*.tab.h io/$*.tab.h
	$(CC) -o io/$*.tab.o -c io/$*.tab.c

io/%.yy.o: io/%.l io/%.tab.o
	$(FLEX) -P$* -oio/$*.yy.c io/$*.l
	$(CC) -o io/$*.yy.o -c io/$*.yy.c -lfl

%.o : %.cpp
	$(CC) -o $*.o -c $*.cpp

.PHONY: clean
clean:
	rm -f */*.o *.o
	rm -f */*.d *.d
	rm -f io/*.tab.c io/*.tab.h io/*.yy.c *.output
	rm -f *.dat $(BFILE) core
	rm -f qt/*.moc.cpp

.PHONY: tags
tags:
	cscope -Rbq
	ctags -R


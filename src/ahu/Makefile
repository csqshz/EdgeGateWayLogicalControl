CC=gcc
APP=ahu
PRJDIR=/mnt/hgfs/ShareDirHost/logical_control
#PRJDIR=/mnt/hgfs/sharedir/logical_control
OUTDIR=/mnt/hgfs/ShareDirHost/logical_control/bin

INCLUDE= \
	-I${PRJDIR}/src/${APP} \
	-I${PRJDIR}/src/${APP}/proc \
	-I${PRJDIR}/src/${APP}/mqtt \
	-I${PRJDIR}/src/${APP}/list \
	-I${PRJDIR}/src/${APP}/ahupid \
	-I${PRJDIR}/src/signal \
	-I${PRJDIR}/src/es_print \
	-I${PRJDIR}/src/pid \
	-I${PRJDIR}/src

LIB=-lpthread -lmosquitto -ljson-c

EXE=aaaa

all:
	${CC} main.c app.c \
	./mqtt/mqttlib.c \
	./proc/proc.c \
	./list/list.c \
	./ahupid/ahupid.c \
	../pid/pid.c \
	../signal/thread_signal.c \
	${INCLUDE} \
	${LIB} \
	-Wall -g \
	-o ${OUTDIR}/${EXE}

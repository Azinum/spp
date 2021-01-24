# makefile
# spp

include config.mk

all: prepare compile run

prepare:
	mkdir -p ${BUILD_DIR}
	mkdir -p ${OUT_DIR}

compile: ${SRC} ${INC}
	${CC} ${FLAGS}

run:
	./${BUILD_DIR}/${PROG} test.c > ${OUT_DIR}/out.c

install:
	${CC} ${FLAGS}
	chmod o+x ${BUILD_DIR}/${PROG}
	cp ${BUILD_DIR}/${PROG} ${INSTALL_DIR}/${PROG}

uninstall:
	rm ${INSTALL_DIR}/${PROG}

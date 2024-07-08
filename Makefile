BUILD_DIR=build
EXE_FILE=coroutines_test
export CC=/usr/bin/gcc-13
export CXX=/usr/bin/g++

all: symlink

build:
	mkdir ${BUILD_DIR}

cache: build
	cmake -B ./${BUILD_DIR} -DCMAKE_BUILD_TYPE=Debug

cmake: cache
	cmake --build ./${BUILD_DIR}/ 

symlink: cmake
	if [ ! -L ${EXE_FILE} ]; then\
		if [ -e ${EXE_FILE} ]; then rm -rf ${EXE_FILE}; fi;\
		ln -s ${BUILD_DIR}/${EXE_FILE} ${EXE_FILE};\
	fi


clean:
	rm -rf ${BUILD_DIR}
	rm -f ${EXE_FILE}

run:
	./${EXE_FILE}
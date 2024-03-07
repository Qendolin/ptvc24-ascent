OS := $(shell uname)
PROJECT_DIR = _project
INSTALL_DIR = _bin

all:
ifeq ($(OS), Darwin)
	cmake -H. -B${PROJECT_DIR} -G "Xcode"
else
	cmake -H. -B${PROJECT_DIR} -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
endif

debug:
	cmake -H. -B${PROJECT_DIR} -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DCMAKE_BUILD_TYPE=Debug
	cmake --build ${PROJECT_DIR} --target install

release:
	cmake -H. -B${PROJECT_DIR} -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release
	cmake --build ${PROJECT_DIR} --target install

install: debug release

ninja-install:
	cmake -H. -B${PROJECT_DIR} -G "Ninja Multi-Config" -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
	cmake --build ${PROJECT_DIR} --target install --config Debug 
	cmake --build ${PROJECT_DIR} --target install --config Release

clean:
	${RM} -r ${PROJECT_DIR}
	${RM} -r ${INSTALL_DIR}

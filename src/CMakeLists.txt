IF(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
ENDIF()

ADD_EXECUTABLE(cdt2cmake cdt2cmake.cpp project.cpp cdtproject.cpp sourcediscovery.cpp tixml_iterator.cpp cdtconfiguration.cpp)
TARGET_LINK_LIBRARIES(cdt2cmake tinyxml)

set(sources
	vrapi_driver.cpp
	vrapi_driver.h
	vrapi_replay.cpp
	vrapi_hooks.cpp
	official/VrApi_Config.h
	official/VrApi_Ext.h
	official/VrApi_Types.h)

add_library(rdoc_vrapi OBJECT ${sources})
target_compile_definitions(rdoc_vrapi ${RDOC_DEFINITIONS})
target_include_directories(rdoc_vrapi ${RDOC_INCLUDES})

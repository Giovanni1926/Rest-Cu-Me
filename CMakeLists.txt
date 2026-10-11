cmake_minimum_required(VERSION 3.21)
project(RestCuMe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)

set(SOURCES
    src/main.cpp
    src/model/restcallparser.cpp
    src/model/callrecord.cpp
    src/model/wsdlparser.cpp
    src/http/httpclientfactory.cpp
    src/http/restclient.cpp
    src/http/requestbodybuilder.cpp
    src/http/historymanager.cpp
    src/flow/flowgraph.cpp
    src/flow/flowfilemanager.cpp
    src/flow/soapcallbuilder.cpp
    src/flow/flowplanner.cpp
    src/flow/jsonpathextractor.cpp
    src/flow/xmlpathextractor.cpp
    src/flow/rcmflowrunservice.cpp
    src/run/rcmrunservice.cpp
    src/ui/flowcanvas.cpp
    src/ui/floweditorview.cpp
    src/ui/mainwindow.cpp
    src/ui/dialogs/flowrequestoptionspanel.cpp
    src/ui/dialogs/flownodedialog.cpp
    src/ui/dialogs/flowsoapnodedialog.cpp
    src/ui/dialogs/flowedgedialog.cpp
    src/ui/dialogs/outputsuggestions.cpp
    src/ui/toolwindow/consolepanel.cpp
    src/ui/toolwindow/historypanel.cpp
    src/ui/widgets/variablesupport.cpp
    src/ui/explorer/explorerpanel.cpp
    src/ui/visual/visualrequestview.cpp
    src/ui/forms/restrequestform.cpp
    src/ui/forms/soaprequestform.cpp
    third_party/pugixml/pugixml.cpp
)

add_executable(RestCuMe WIN32 ${SOURCES})

target_include_directories(RestCuMe PRIVATE
    src
    third_party/nlohmann
    third_party/pugixml
)

target_link_libraries(RestCuMe PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

if(WIN32)
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake}" DIRECTORY)
    find_program(_windeployqt windeployqt HINTS "${_qt_bin_dir}")
    if(_windeployqt)
        add_custom_command(TARGET RestCuMe POST_BUILD
            COMMAND "${_windeployqt}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                    "$<TARGET_FILE:RestCuMe>"
            COMMENT "Deploying Qt runtime libraries"
        )
    endif()
endif()

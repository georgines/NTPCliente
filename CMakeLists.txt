add_library(NTPCliente STATIC
    src/NTPCliente.cpp
)

if(NOT TARGET Wifi)
    message(FATAL_ERROR "A biblioteca NTPCliente depende da Wifi. Certifique-se de chamar add_subdirectory(Wifi) antes da NTPCliente.")
endif()

target_include_directories(NTPCliente
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src 

)

if(TARGET FreeRTOS)
    target_link_libraries(NTPCliente
        PUBLIC
            Wifi
            FreeRTOS
    )
else()
     target_link_libraries(NTPCliente
        PUBLIC
            Wifi          
    )
endif()

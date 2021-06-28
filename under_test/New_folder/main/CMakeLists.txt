set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../webpages")
idf_component_register(SRCS     "app_flash.c"
                                "app_httpServer.c"
                                "app_wifiSta.c"
                                "app_wifiAp.c"
                                
                                "device_sntp.c"
                                "app_mqttClient.c" 
                                "app_consolNodeCmd.c" 
                                "app_consol.c"
                                
                                "app_process_input.c"
                                "app_take_input.c"
                                "app_main.c"
                            
                           
                    INCLUDE_DIRS "." "include"
                    EMBED_FILES "${WEB_SRC_DIR}/setup/setup.html")

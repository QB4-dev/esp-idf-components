set(COMPONENT_SRCS "main.c level_crossing.c lights.c schedule.c server.c servo.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES esp_http_server_gen nvs_flash mdns esp_wav_player)

register_component()
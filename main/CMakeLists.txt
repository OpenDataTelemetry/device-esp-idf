idf_component_register(SRCS "app_main.c"
  "nmea_parser.c"
  # REQUIRES esp_event esp_driver_uart
  INCLUDE_DIRS ".")

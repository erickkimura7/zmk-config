if(CONFIG_ZMK_FORCE_DCDC_HV_OFF OR CONFIG_ZMK_FORCE_DCDC_OFF)
  zephyr_library()
  zephyr_library_sources(src/force_dcdc_off.c)
endif()

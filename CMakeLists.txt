if(CONFIG_ZMK_FORCE_DCDC_HV_OFF OR CONFIG_ZMK_FORCE_DCDC_OFF OR CONFIG_ZMK_RESET_REASON_LOG OR CONFIG_ZMK_KEEP_HFXO_ON)
  zephyr_library()
  zephyr_library_sources_ifdef(CONFIG_ZMK_FORCE_DCDC_HV_OFF src/force_dcdc_off.c)
  if(CONFIG_ZMK_FORCE_DCDC_OFF AND NOT CONFIG_ZMK_FORCE_DCDC_HV_OFF)
    zephyr_library_sources(src/force_dcdc_off.c)
  endif()
  zephyr_library_sources_ifdef(CONFIG_ZMK_RESET_REASON_LOG src/reset_reason_log.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_KEEP_HFXO_ON src/keep_hfxo_on.c)
endif()

# Uses ZMK's own headers (zmk/*.h), which are private to the "app" target,
# so it is compiled into app instead of a separate Zephyr library.
target_sources_ifdef(CONFIG_ZMK_MAC_PROFILE app PRIVATE src/mac_profile.c)

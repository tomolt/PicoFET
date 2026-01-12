#if defined(RASPBERRYPI_PICO) || defined(RASPBERRYPI_PICO_W) \
 || defined(RASPBERRYPI_PICO2) || defined(RASPBERRYPI_PICO2_W)

#  ifndef PIN_TCK
#    define PIN_TCK 8
#  endif

#  ifndef PIN_TMS
#    define PIN_TMS 9
#  endif

#  ifndef PIN_TST
#    define PIN_TST 10
#  endif

#  ifndef PIN_RST
#    define PIN_RST 11
#  endif

#  ifndef PIN_TDO
#    define PIN_TDO 12
#  endif

#  ifndef PIN_TDI
#    define PIN_TDI 13
#  endif

#elif defined(SEEED_XIAO_RP2040) || defined(SEEED_XIAO_RP2350)

#  ifndef PIN_TCK
#    define PIN_TCK 27
#  endif

#  ifndef PIN_TMS
#    define PIN_TMS 28
#  endif

#  ifndef PIN_TST
#    define PIN_TST 3
#  endif

#  ifndef PIN_RST
#    define PIN_RST 4
#  endif

#  ifndef PIN_TDO
#    define PIN_TDO 5
#  endif

#  ifndef PIN_TDI
#    define PIN_TDI 6
#  endif

#else

#  if !defined(PIN_TCK) || !defined(PIN_TMS) \
   || !defined(PIN_TST) || !defined(PIN_RST) \
   || !defined(PIN_TDO) || !defined(PIN_TDI)
#    error "No pin assignment specified for this board type."
#  endif

#endif

#ifndef PIN_SBWTCK
#  define PIN_SWBTCK PIN_TST
#endif

#ifndef PIN_SBWTDIO
#  define PIN_SBWTDIO PIN_RST
#endif

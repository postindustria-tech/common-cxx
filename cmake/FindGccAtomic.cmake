set(ATOMIC_SEARCH_PATH
  $ENV{HOME}/local/lib64
  $ENV{HOME}/local/lib
  /usr/local/lib64
  /usr/local/lib
  /opt/local/lib64
  /opt/local/lib
  /usr/lib64
  /usr/lib
  /lib64
  /lib
)
if (32bit)
set(ATOMIC_SEARCH_PATH
  ${ATOMIC_SEARCH_PATH}
  $ENV{HOME}/local/lib32
  /usr/local/lib32
  /opt/local/lib32
  /usr/lib32
  /lib32
)
endif()



FIND_LIBRARY(GCCLIBATOMIC_LIBRARY NAMES atomic atomic.so.1 libatomic.so.1
  HINTS
  ${CMAKE_LIBRARY_PATH}
  ${ATOMIC_SEARCH_PATH}
)

IF (GCCLIBATOMIC_LIBRARY)
    SET(GCCLIBATOMIC_FOUND TRUE)
    MESSAGE(STATUS "Found GCC's libatomic.so: lib=${GCCLIBATOMIC_LIBRARY}")
ELSE ()
    SET(GCCLIBATOMIC_FOUND FALSE)
    MESSAGE(STATUS "GCC's libatomic was not found.")
    MESSAGE(STATUS "Either upgrade to GCC 5+, or install libatomic with 'sudo apt-get install libatomic1'")
ENDIF ()

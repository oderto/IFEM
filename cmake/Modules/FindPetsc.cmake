FIND_PATH(
  PETSC_INCLUDE_DIR
  NAMES petsc.h
  PATHS /usr/include/petsc/
  $ENV{PETSC_DIR}/include
  )
FIND_PATH(
  PETSCCONF_INCLUDE_DIR
  NAMES petscconf.h
  PATHS /usr/include/petsc/ 
  $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/include
  )

IF( PETSCCONF_INCLUDE_DIR EQUAL PETSC_INCLUDE_DIR )
  SET(CMAKE_REQUIRED_INCLUDES "${PETSC_INCLUDE_DIR};${CMAKE_REQUIRED_INCLUDES}")
ELSE( PETSCCONF_INCLUDE_DIR EQUAL PETSC_INCLUDE_DIR )
  SET(CMAKE_REQUIRED_INCLUDES "${PETSC_INCLUDE_DIR};${PETSCCONF_INCLUDE_DIR};${CMAKE_REQUIRED_INCLUDES}")
ENDIF( PETSCCONF_INCLUDE_DIR EQUAL PETSC_INCLUDE_DIR )

#MESSAGE( STATUS "CMAKE_REQUIRED_INCLUDES: ${CMAKE_REQUIRED_INCLUDES}")

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(petscconf.h HAVE_PETSCCONF_H)
CHECK_INCLUDE_FILES(petsc.h HAVE_PETSC_H)
IF ( HAVE_PETSC_H )
  ADD_DEFINITIONS( -DHAVE_PETSC_H -DHAVE_PETSC )
ENDIF( HAVE_PETSC_H )

#FIND_LIBRARY(PETSC_LIB_MPIUNI    mpiuni    PATHS /usr/local/petsc-2.3.3-p8/lib/linux-gnu-c-debug QUIET )
FIND_LIBRARY(PETSC_LIB_PETSC     petsc     PATHS /usr/lib $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib)
FIND_LIBRARY(PETSC_LIB_PETSCDM   petscdm   PATHS /usr/lib $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib)
FIND_LIBRARY(PETSC_LIB_PETSCVEC  petscvec  PATHS /usr/lib $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib)
FIND_LIBRARY(PETSC_LIB_PETSCMAT  petscmat  PATHS /usr/lib $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib)
FIND_LIBRARY(PETSC_LIB_PETSCKSP  petscksp  PATHS /usr/lib $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib)
FIND_LIBRARY(PETSC_LIB_PETSCSNES petscsnes PATHS /usr/lib $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/lib)

IF(PETSC_LIB_PETSC)
  SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_LIB_PETSC})
ENDIF(PETSC_LIB_PETSC)

IF(PETSC_LIB_PETSCDM)
  SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_LIB_PETSCDM})
ENDIF(PETSC_LIB_PETSCDM)

IF(PETSC_LIB_PETSCVEC)
  SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_LIB_PETSCVEC})
ENDIF(PETSC_LIB_PETSCVEC)

IF(PETSC_LIB_PETSCMAT)
  SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_LIB_PETSCMAT})
ENDIF(PETSC_LIB_PETSCMAT)

IF(PETSC_LIB_PETSCKSP)
  SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_LIB_PETSCKSP})
ENDIF(PETSC_LIB_PETSCKSP)

IF(PETSC_LIB_PETSCSNES)
  SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_LIB_PETSCSNES})
ENDIF(PETSC_LIB_PETSCSNES)

exec_program("cat $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/conf/petscvariables | grep PACKAGES_LIBS | awk -F  ' = ' '{print $2}'" OUTPUT_VARIABLE PETSC_VARIABLES)
SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_VARIABLES})

exec_program("cat $ENV{PETSC_DIR}/$ENV{PETSC_ARCH}/conf/petscvariables | grep \"PCC_LINKER_LIBS =\"| awk -F  ' = ' '{print $2}'" OUTPUT_VARIABLE PETSC_VARIABLES)
SET(PETSC_LIBRARIES ${PETSC_LIBRARIES} ${PETSC_VARIABLES})


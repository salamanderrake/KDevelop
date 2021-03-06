# Try to find KDeclarative includes

find_path(KDECLARATIVE_INCLUDES kdeclarative.h)
find_library(KDECLARATIVE_LIBRARY kdeclarative)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(KDeclarative DEFAULT_MSG KDECLARATIVE_INCLUDES KDECLARATIVE_LIBRARY)

MARK_AS_ADVANCED(KDECLARATIVE_INCLUDES)


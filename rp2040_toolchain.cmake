# Set the system name to Generic, indicating cross-compilation
set(CMAKE_SYSTEM_NAME Generic)

# Specify the cross-compiler locations
set(CMAKE_C_COMPILER /opt/homebrew/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /opt/homebrew/bin/arm-none-eabi-g++)

# Specify compiler flags, if necessary
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")

# Prevent CMake from testing the compiler's ability to compile executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)


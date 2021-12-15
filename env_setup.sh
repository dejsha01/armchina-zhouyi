#!/bin/sh
###
# CONFIG_DRV_*:  users outside ArmChina AI team should set those variables before using
# COMPASS_DRV_*: users outside ArmChina AI team should validate those variables depend on CONFIG values
#
# BTENVAR: Buildtime Environment Variables
# RTENVAR: Runtime Environment Variables
# BRENVAR: Buildtime & Runtime Environment Variables
###

### configure kernel path for target platform:
###    CONFIG_DRV_BTENVAR_[platform]_[kernel major]_[kernel minor]_KPATH
setenv CONFIG_DRV_RTENVAR_SIM_PATH           [*** your private path ***]
setenv CONFIG_DRV_BRENVAR_Z5_SIM_LPATH       [*** your private path ***]
setenv CONFIG_DRV_BRENVAR_X86_CLPATH         [*** your private path ***]
setenv CONFIG_DRV_BTENVAR_CROSS_CXX_PATH     [*** your private path ***]
setenv CONFIG_DRV_BTENVAR_JUNO_KPATH         [*** your private path ***]
setenv CONFIG_DRV_BTENVAR_6CG_KPATH          [*** your private path ***]

### other configs
setenv CONFIG_DRV_RTENVAR_PY_INCD_PATH       [*** your private path ***]
setenv CONFIG_DRV_RTENVAR_NUMPY_INCD_PATH    [*** your private path ***]

### Toolchains
### add toolchain/kernel path of your supported platform(s)
# Target platform: x86
setenv COMPASS_DRV_BTENVAR_X86_CXX           g++

# Target platform: arm64
setenv COMPASS_DRV_BTENVAR_CROSS_CXX         aarch64-linux-gnu-g++
setenv COMPASS_DRV_BTENVAR_ARCH              arm64
setenv COMPASS_DRV_BTENVAR_CROSS_COMPILE     aarch64-linux-
setenv COMPASS_DRV_BTENVAR_CROSS_COMPILE_GNU aarch64-linux-gnu-

### Z1/2/3 Simulation
setenv COMPASS_DRV_RTENVAR_Z1_SIMULATOR      ${CONFIG_DRV_RTENVAR_SIM_PATH}/aipu_simulator_z1
setenv COMPASS_DRV_RTENVAR_Z2_SIMULATOR      ${CONFIG_DRV_RTENVAR_SIM_PATH}/aipu_simulator_z2
setenv COMPASS_DRV_RTENVAR_Z3_SIMULATOR      ${CONFIG_DRV_RTENVAR_SIM_PATH}/aipu_simulator_z3
setenv COMPASS_DRV_RTENVAR_SIM_LPATH         ${CONFIG_DRV_RTENVAR_SIM_PATH}/lib
setenv LD_LIBRARY_PATH                       ${COMPASS_DRV_RTENVAR_SIM_LPATH}:$LD_LIBRARY_PATH

### Z5 Simulation
setenv COMPASS_DRV_BRENVAR_Z5_SIM_LNAME      aipu_simulator
setenv LD_LIBRARY_PATH                       ${CONFIG_DRV_BRENVAR_Z5_SIM_LPATH}:$LD_LIBRARY_PATH


### You are not suggested to modify the following environment variables
# Driver src & build dir.
setenv COMPASS_DRV_BTENVAR_UMD_DIR           driver/umd
setenv COMPASS_DRV_BTENVAR_KMD_DIR           driver/kmd
setenv COMPASS_DRV_BTENVAR_TEST_DIR          ./test

setenv COMPASS_DRV_BRENVAR_ODIR_TOP          `pwd`/bin
setenv COMPASS_DRV_BTENVAR_UMD_BUILD_DIR     `pwd`/build/umd
setenv COMPASS_DRV_BTENVAR_KMD_BUILD_DIR     `pwd`/build/kmd
setenv COMPASS_DRV_BTENVAR_TEST_BUILD_DIR    `pwd`/build/test

# Driver naming
setenv COMPASS_DRV_BTENVAR_UMD_V_MAJOR       0
setenv COMPASS_DRV_BTENVAR_UMD_V_MINOR       0.1
setenv COMPASS_DRV_BTENVAR_UMD_SO_NAME       libz5aipudrv.so
setenv COMPASS_DRV_BTENVAR_UMD_SO_NAME_MAJOR ${COMPASS_DRV_BTENVAR_UMD_SO_NAME}.${COMPASS_DRV_BTENVAR_UMD_V_MAJOR}
setenv COMPASS_DRV_BTENVAR_UMD_SO_NAME_FULL  ${COMPASS_DRV_BTENVAR_UMD_SO_NAME_MAJOR}.${COMPASS_DRV_BTENVAR_UMD_V_MINOR}
setenv COMPASS_DRV_BTENVAR_KMD_VERSION       3.2.3

setenv COMPASS_DRV_BRENVAR_ERROR             "\033[31;1m[DRV ERROR]\033[0m"
setenv COMPASS_DRV_BRENVAR_WARN              "\033[31;1m[DRV WARN]\033[0m"
setenv COMPASS_DRV_BRENVAR_INFO              "\033[32;1m[DRV INFO]\033[0m"

###
setenv COMPASS_DRV_BRENVAR_SETUP_DONE        yes

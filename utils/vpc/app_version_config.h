// Copyright (c) 2024 The Source Authors.  All rights reserved.
//
// App version configuration.

#ifndef VPC_UTILS_VPC_APP_VERSION_CONFIG_H_
#define VPC_UTILS_VPC_APP_VERSION_CONFIG_H_

#include "product_version_config.h"

// App long product name.
#define SE_PRODUCT_NAME_STRING "VPC"

// App file version header.
#define SE_APP_VER_FILE_VERSION_HEADER SE_VER_PRODUCT_VERSION_HEADER

// App file version info.
#define SE_APP_VER_FILE_VERSION_INFO SE_VER_PRODUCT_VERSION_INFO

// App file version as string.
#define SE_PRODUCT_FILE_VERSION_INFO_STRING \
  SE_STRINGIFY(SE_APP_VER_FILE_VERSION_INFO)

// App file description.
#ifndef SE_PRODUCT_FILE_DESCRIPTION_STRING
#define SE_PRODUCT_FILE_DESCRIPTION_STRING ""
#endif

#ifdef _WIN32
// App file internal name.  Should be the original filename, without
// extension.
// #define SE_PRODUCT_INTERNAL_NAME_STRING ""

// App file original name.  This information enables an application to
// determine whether a file has been renamed by a user.
// #define SE_PRODUCT_ORIGINAL_NAME_STRING \
//  SE_PRODUCT_INTERNAL_NAME_STRING ""
#endif  // _WIN32

#endif  // !VPC_UTILS_VPC_APP_VERSION_CONFIG_H_

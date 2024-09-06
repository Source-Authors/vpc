// Copyright (c) 2024 The Source Authors.  All rights reserved.
//
// App version configuration.

#ifndef VPC_UTILS_VPC_APP_VERSION_CONFIG_H_
#define VPC_UTILS_VPC_APP_VERSION_CONFIG_H_

#include "product_version_config.h"

// App long product name.
#define SRC_PRODUCT_NAME_STRING "VPC"

// App file version header.
#define SRC_APP_VER_FILE_VERSION_HEADER SRC_VER_PRODUCT_VERSION_HEADER

// App file version info.
#define SRC_APP_VER_FILE_VERSION_INFO SRC_VER_PRODUCT_VERSION_INFO

// App file version as string.
#define SRC_PRODUCT_FILE_VERSION_INFO_STRING \
  SRC_STRINGIFY(SRC_APP_VER_FILE_VERSION_INFO)

// App file description.
#ifndef SRC_PRODUCT_FILE_DESCRIPTION_STRING
#define SRC_PRODUCT_FILE_DESCRIPTION_STRING ""
#endif

#ifdef _WIN32
// App file internal name.  Should be the original filename, without
// extension.
// #define SRC_PRODUCT_INTERNAL_NAME_STRING ""

// App file original name.  This information enables an application to
// determine whether a file has been renamed by a user.
// #define SRC_PRODUCT_ORIGINAL_NAME_STRING \
//  SRC_PRODUCT_INTERNAL_NAME_STRING ""
#endif  // _WIN32

#endif  // !VPC_UTILS_VPC_APP_VERSION_CONFIG_H_

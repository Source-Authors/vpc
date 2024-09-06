// Copyright (c) 2024 The Source Authors.  All rights reserved.
//
// Product version info configuration.

#ifndef VPC_UTILS_VPC_PRODUCT_VERSION_CONFIG_H_
#define VPC_UTILS_VPC_PRODUCT_VERSION_CONFIG_H_

// Company long name.
#define SRC_PRODUCT_COMPANY_NAME_STRING "The Source Authors"

// Company short name.
#define SRC_PRODUCT_COMPANY_SHORT_NAME_INFO_STRING "SourceAuthors"

// Copyright.
#define SRC_PRODUCT_LEGAL_COPYRIGHT_STRING \
  "Copyright " SRC_PRODUCT_COMPANY_NAME_STRING ". All rights reserved."

#ifndef SRC_STRINGIFY_WORKER
// Stringify worker.
#define SRC_STRINGIFY_WORKER(text) #text
#endif

#ifndef SRC_STRINGIFY
// Stringify.
#define SRC_STRINGIFY(text) SRC_STRINGIFY_WORKER(text)
#endif

// clang-format off

// Product version header.
#define SRC_VER_PRODUCT_VERSION_HEADER \
  1,3,2,920  // NOLINT Version

// Product version info.
#define SRC_VER_PRODUCT_VERSION_INFO \
  1.3.2.920  // NOLINT Version

// clang-format on

// Product version info as string.
#define SRC_PRODUCT_VERSION_INFO_STRING \
  SRC_STRINGIFY(SRC_VER_PRODUCT_VERSION_INFO)

// File version header.
#define SRC_VER_FILEVERSION_HEADER SRC_VER_PRODUCT_VERSION_HEADER

// File version info.
#define SRC_VER_FILEVERSION_INFO SRC_VER_PRODUCT_VERSION_INFO

// File version info string.
#define SRC_PRODUCT_FILEVERSION_INFO_STRING \
  SRC_STRINGIFY(SRC_VER_FILEVERSION_INFO)

// All file flags mask.
#define SRC_VER_FFI_FILEFLAGSMASK VS_FFI_FILEFLAGSMASK

// Debug flag.
#define SRC_VER_FF_DEBUG VS_FF_DEBUG

#ifndef _DEBUG
// Release file flags.
#define SRC_VER_FILEFLAGS 0
#else
// Debug file flags.
#define SRC_VER_FILEFLAGS SRC_VER_FF_DEBUG
#endif

#endif  // !VPC_UTILS_VPC_PRODUCT_VERSION_CONFIG_H_

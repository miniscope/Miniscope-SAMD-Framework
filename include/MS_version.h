/**
@file
@brief Firmware version announced in the buffer header (VERSION_SIDEBAND_ENABLE, see README).
Bump FW_VERSION_* whenever the stream format or the acquisition behaviour changes, so that
miniscope-io can check that firmware, mio version and FPGA bitfile belong together.
The git hash comes from MS_version_git.h, written by script/MS_prebuild.ps1; builds that do not
run the pre-build script report hash 0 (unknown).
*/

#ifndef MS_VERSION_H_
#define MS_VERSION_H_

// Semantic version, the next after release tag v0.3.1; 0.4.0 adds the 13-word header with CRC word;
// 0.5.0 adds the black reference word and the PYTHON480 temperature (header layout 4, 14 words);
// 0.6.0 replaces the word layout with a protobuf header (HEADER_PROTOBUF_ENABLE, proto/wlms_header.proto).
#define FW_VERSION_MAJOR	0
#define FW_VERSION_MINOR	8
#define FW_VERSION_PATCH	0
// Decode key in the buffer header: a header change bumps at least FW_VERSION_MINOR.
#define FW_ID				((FW_VERSION_MAJOR << 8) | FW_VERSION_MINOR)

#if defined(__has_include)
#if __has_include("MS_version_git.h")
#include "MS_version_git.h"
#endif
#endif

#ifndef FW_GIT_HASH
#define FW_GIT_HASH		0x00000000UL	// unknown: the build did not run MS_prebuild.ps1
#endif
#ifndef FW_GIT_DIRTY
#define FW_GIT_DIRTY	0
#endif

#endif /* MS_VERSION_H_ */

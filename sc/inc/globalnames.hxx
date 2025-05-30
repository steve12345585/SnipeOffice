/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>

inline constexpr OUString STR_DB_LOCAL_NONAME = u"__Anonymous_Sheet_DB__"_ustr;
inline constexpr OUString STR_DB_GLOBAL_NONAME = u"__Anonymous_DB__"_ustr;

inline constexpr OUString STR_GLOBAL_RANGE_NAME = u"__Global_Range_Name__"_ustr;

#define TEXTWIDTH_DIRTY 0xffff

#define DATE_TIME_FACTOR 86400.0

// Device name used to represent the software group interpreter for OpenCL
// mode. This string gets stored in use configuration as the device name.
#define OPENCL_SOFTWARE_DEVICE_CONFIG_NAME "Software"

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

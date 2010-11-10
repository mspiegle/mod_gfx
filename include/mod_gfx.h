/*
*
* mod_gfx.h - A support header for mod_gfx
* Copyright (C) 2010 Michael Spiegle (mike@nauticaltech.com)
*
* This file is part of mod_gfx.
* 
* mod_gfx is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* mod_gfx is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef _MOD_GFX_H_
#define _MOD_GFX_H_

#include <gd.h>

#include <httpd.h>
#include <http_config.h>
#include <util_filter.h>

#include <apr_hash.h>
#include <apr_errno.h>
#include <apr_pools.h>
#include <apr_buckets.h>

typedef struct {
	apr_bucket_brigade* temp_brigade;
} gfx_filter_ctx_t;

typedef enum {
	IMAGE_TYPE_JPG = 0x20,
	IMAGE_TYPE_PNG,
	IMAGE_TYPE_GIF,
	IMAGE_TYPE_SRC
} gfx_image_type_t;

typedef enum {
	IMAGE_ACTION_RESIZE = 0x40,
	IMAGE_ACTION_RESAMPLE,
	IMAGE_ACTION_CROP,
	IMAGE_ACTION_WATERMARK,
	IMAGE_ACTION_NOOP
} gfx_action_t;

const char IMAGE_JPG_SIG[] = { (char)0xFF, (char)0xD8, (char)0xFF };
const char IMAGE_GIF_SIG[] = { (char)0x47, (char)0x49, (char)0x46 };
const char IMAGE_PNG_SIG[] = { (char)0x89, (char)0x50, (char)0x4E, (char)0x47,
                               (char)0x0D, (char)0x0A, (char)0x1A, (char)0x0A };

#define IMAGE_JPG_SIZE 3
#define IMAGE_GIF_SIZE 3
#define IMAGE_PNG_SIZE 8

static void log_bucket_type(ap_filter_t* f, apr_bucket* b);
static void debug_brigade(ap_filter_t* f, apr_bucket_brigade* bb);
static gdImagePtr gd_from_blob(char*, apr_size_t, gfx_image_type_t*);
static int get_image_type(char* buffer, apr_size_t);

#endif
// vim: ts=2

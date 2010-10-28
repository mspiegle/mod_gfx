/*
*
* mod_gfx.c - The main implementation of mod_gfx
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

#include "mod_gfx.h"

#include <gd.h>

#include <httpd.h>
#include <http_log.h>
#include <util_filter.h>

#include <apr_buckets.h>

static int
filter(ap_filter_t* f, apr_bucket_brigade* bb) {
	ap_log_error(APLOG_MARK, APLOG_INFO, 0, f->r->server,
	             "gfx_filter(): This is a test");

	return ap_pass_brigade(f->next, bb);
}

static void
register_hooks(apr_pool_t* p) {
	ap_register_output_filter("GFX", filter, NULL, AP_FTYPE_CONTENT_SET);
}

module AP_MODULE_DECLARE_DATA gfx_module = {
	STANDARD20_MODULE_STUFF,
	NULL,          //abused container creator
	NULL,          //i'm too lazy to implement merging
	NULL,          //create server config
	NULL,          //i'm too lazy to implement merging
	NULL,          //command table
	register_hooks //hooks
};

// vim: ts=2

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
	ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
	             "filter(): Called");

	gfx_filter_ctx_t* ctx = f->ctx;
	apr_bucket* b;

	//short circuit for empty responses
	if (APR_BRIGADE_EMPTY(bb)) {
		return ap_pass_brigade(f->next, bb);
	}

	//if we didn't have a pre-existing context, then we can create it here
	//and initialize it.
	if (ctx == NULL) {
		f->ctx = ctx = apr_pcalloc(f->r->pool, sizeof(gfx_filter_ctx_t));

		//we have a temporary bucket brigade to store data in over subsequent calls
		//to the filter.
		ctx->tempbb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
	}

	//let's do a loopy loop
	//while ((b = APR_BRIGADE_FIRST(bb)) && (b != APR_BRIGADE_SENTINEL(bb))) {
	for (b = APR_BRIGADE_FIRST(bb);
	     b != APR_BRIGADE_SENTINEL(bb);
			 b = APR_BUCKET_NEXT(b)) {
		if (APR_BUCKET_IS_EOS(b)) {
			ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
			             "filter(): Got EOS bucket");
		}
		if (APR_BUCKET_IS_FLUSH(b)) {
			ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
			             "filter(): Got FLUSH bucket");
		}
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): Length = [%lu]", b->length);
	}

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

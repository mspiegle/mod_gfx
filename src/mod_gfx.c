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
		ctx->temp_brigade = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
	}

	//let's do a loopy loop
	for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); 
	     b = APR_BUCKET_NEXT(b)) {

		//tell us what kind of bucket we have (for debugging)
		log_bucket_type(f, b);

		//if we do get a sentinel, then we should be able to resize the image
		if (APR_BUCKET_IS_EOS(b)) {
			ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
			             "filter(): returning due to EOS");
			APR_BRIGADE_INSERT_TAIL(ctx->temp_brigade, b);
			return ap_pass_brigade(f->next, ctx->temp_brigade);
		}

		//we aren't going to pass meta buckets because we can't do much with them 
		//until we're done anyways
		if (APR_BUCKET_IS_METADATA(b)) {
			ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
			             "filter(): skipping bucket");
			APR_BUCKET_REMOVE(b);
			continue;
		}
		
		//we need to build up our temp_brigade.  first, make a copy of the bucket
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): copying bucket");
		apr_status_t ret;
		apr_bucket* temp;
		if (APR_SUCCESS != (ret = apr_bucket_copy(b, &temp))) {
			ap_log_error(APLOG_MARK, APLOG_CRIT, 0, f->r->server,
			             "filter(): failed to copy bucket");
			return ret;
		}

		//next, ensure that bucket goes to the heap
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): setaside bucket");
		if (APR_SUCCESS != (ret = apr_bucket_setaside(temp, f->r->pool))) {
			ap_log_error(APLOG_MARK, APLOG_CRIT, 0, f->r->server,
			             "filter(): failed to setaside bucket");
			return ret;
		}

		//now we can add the bucket to our temp brigade
		APR_BRIGADE_INSERT_TAIL(ctx->temp_brigade, temp);
	}
	//return ap_pass_brigade(f->next, bb);
	ap_log_error(APLOG_MARK, APLOG_CRIT, 0, f->r->server,
	             "filter(): if we got here, then the loop is done");
}

static void
log_bucket_type(ap_filter_t* f, apr_bucket* b) {
if (APR_BUCKET_IS_EOS(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=EOS len=[%lu]", b->length);
	}
	if (APR_BUCKET_IS_FLUSH(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=FLUSH len=[%lu]", b->length);
	}
	
	if (APR_BUCKET_IS_FILE(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=FILE len=[%lu]", b->length);
	}
	
	if (APR_BUCKET_IS_PIPE(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=PIPE len=[%lu]", b->length);
	}
	
	if (APR_BUCKET_IS_HEAP(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=HEAP len=[%lu]", b->length);
	}
	
	if (APR_BUCKET_IS_TRANSIENT(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=TRANSIENT len=[%lu]", b->length);
	}
	
	if (APR_BUCKET_IS_IMMORTAL(b)) {
		ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
		             "filter(): type=IMMORTAL len=[%lu]", b->length);
	}
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

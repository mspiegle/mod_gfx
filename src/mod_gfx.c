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
	//short circuit for empty responses
	if (APR_BRIGADE_EMPTY(bb)) {
		return ap_pass_brigade(f->next, bb);
	}

	//if we didn't have a pre-existing context, then we can create it here
	//and initialize it.
	gfx_filter_ctx_t* ctx = f->ctx;
	if (ctx == NULL) {
		//get a new config
		f->ctx = ctx = apr_pcalloc(f->r->pool, sizeof(gfx_filter_ctx_t));

		//we have a temporary bucket brigade to store data in over subsequent calls
		//to the filter.
		ctx->temp_brigade = apr_brigade_create(f->r->pool, f->c->bucket_alloc);
	}

	//let's do a loopy loop
	apr_bucket* b;
	for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb); 
	     b = APR_BUCKET_NEXT(b)) {

		//EOS received, perform our profile operations
		if (APR_BUCKET_IS_EOS(b)) {
			ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
			             "filter(): EOS received");

			//we don't need the EOS bucket... we can create one later
			APR_BUCKET_REMOVE(b);

			//we need to get the data as a single buffer
			apr_status_t ret;
			char* buffer = NULL;
			apr_size_t buffer_length = 0;
			ret = apr_brigade_pflatten(ctx->temp_brigade, &buffer, &buffer_length,
			                           f->r->pool);
			if (ret != APR_SUCCESS) {
				ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
				             "filter(): pflatten() failed");
				return ret;
			}

			//make a gd object from the source data
			//TODO: register cleanup function for blob
			gdImagePtr src_image = NULL;
			gfx_image_type_t image_type;
			src_image = gd_from_blob(buffer, buffer_length, &image_type);
			if (NULL == src_image) {
				ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, f->r->server,
				             "filter(): gd_from_blob() failed");
				//TODO: what's the right way to handle errors in the filter?
				return APR_EGENERAL;
			}

			//make a gd object for the output data
			gdImagePtr dst_image = NULL;
			dst_image = gdImageCreateTrueColor(250, 250);
			gdImageCopyResized(dst_image, src_image, 0, 0, 0, 0, 250, 250, 250, 250);


			return ap_pass_brigade(f->next, ctx->temp_brigade);
		}

		//we don't really care about metadata buckets
		if (APR_BUCKET_IS_METADATA(b)) {
			APR_BUCKET_REMOVE(b);
			continue;
		}
		
		//we need to build up our temp_brigade.  first, make a copy of the bucket
		//TODO: let's see if we can just add the bucket to temp_brigade, then
		//remove it from bb.  might be faster than copying and stuff.
		apr_status_t ret;
		apr_bucket* temp;
		if (APR_SUCCESS != (ret = apr_bucket_copy(b, &temp))) {
			ap_log_error(APLOG_MARK, APLOG_CRIT, 0, f->r->server,
			             "filter(): failed to copy bucket");
			return ret;
		}

		//next, ensure that bucket goes somewhere semi-permanent
		if (APR_SUCCESS != (ret = apr_bucket_setaside(temp, f->r->pool))) {
			ap_log_error(APLOG_MARK, APLOG_CRIT, 0, f->r->server,
			             "filter(): failed to setaside bucket");
			return ret;
		}

		//now we can add the bucket to our temp brigade
		APR_BRIGADE_INSERT_TAIL(ctx->temp_brigade, temp);
	}
	//TODO: this might not be right - figure it out
	return ap_pass_brigade(f->next, bb);
}

static gdImagePtr
gd_from_blob(char* buffer, apr_size_t buffer_length,
             gfx_image_type_t* image_type) {
	//this simple function creates a gd object based on the original source type
	//make sure the passed image_type is somewhat valid
	if (image_type == NULL) {
		return NULL;
	}

	//figure out the image type and get us the appropriate gd object
	switch (get_image_type(buffer, buffer_length)) {
		case IMAGE_TYPE_JPG:
			return gdImageCreateFromJpegPtr(buffer_length, buffer);
			break;

		case IMAGE_TYPE_PNG:
			return gdImageCreateFromPngPtr(buffer_length, buffer);
			break;

		case IMAGE_TYPE_GIF:
			return gdImageCreateFromGifPtr(buffer_length, buffer);
			break;
	}

	return NULL;
}

static int
get_image_type(char* buffer, apr_size_t buffer_length) {
	//fast magic check to see what type an image is
	if (buffer == NULL) {
		return -1;
	}

	//check for jpeg
	if (buffer_length >= IMAGE_JPG_SIZE) {
		if (memcmp(buffer, IMAGE_JPG_SIG, IMAGE_JPG_SIZE) == 0) {
			return IMAGE_TYPE_JPG;
		}
	}

	//check for png
	if (buffer_length >= IMAGE_PNG_SIZE) {
		if (memcmp(buffer, IMAGE_PNG_SIG, IMAGE_PNG_SIZE) == 0) {
			return IMAGE_TYPE_PNG;
		}
	}

	//check for gif
	if (buffer_length >= IMAGE_GIF_SIZE) {
		if (memcmp(buffer, IMAGE_GIF_SIG, IMAGE_GIF_SIZE) == 0) {
			return IMAGE_TYPE_GIF;
		}
	}
	
	return -1;
}

static void
debug_brigade(ap_filter_t* f, apr_bucket_brigade* bb) {
	//just do a quick loop and print out the bucket types
	apr_bucket* b;
	for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb);
	     b = APR_BUCKET_NEXT(b)) {
		log_bucket_type(f, b);
	}
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

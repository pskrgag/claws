/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto & the Sylpheed-Claws team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef USE_GPGME

#include "defs.h"
#include <glib.h>
#include <gpgme.h>

#include "utils.h"
#include "privacy.h"
#include "procmime.h"
#include "pgpmime.h"
#include "sgpgme.h"
#include "prefs_common.h"

typedef struct _PrivacyDataPGP PrivacyDataPGP;

struct _PrivacyDataPGP
{
	PrivacyData	data;
	
	gboolean	done_sigtest;
	gboolean	is_signed;
	GpgmeSigStat	sigstatus;
	GpgmeCtx 	ctx;
};

static PrivacySystem pgpmime_system;

static gint pgpmime_check_signature(MimeInfo *mimeinfo);

static PrivacyDataPGP *pgpmime_new_privacydata()
{
	PrivacyDataPGP *data;

	data = g_new0(PrivacyDataPGP, 1);
	data->data.system = &pgpmime_system;
	data->done_sigtest = FALSE;
	data->is_signed = FALSE;
	data->sigstatus = GPGME_SIG_STAT_NONE;
	gpgme_new(&data->ctx);
	
	return data;
}

static void pgpmime_free_privacydata(PrivacyData *_data)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) _data;
	
	g_free(data);
}

static gboolean pgpmime_is_signed(MimeInfo *mimeinfo)
{
	MimeInfo *parent;
	MimeInfo *signature;
	gchar *protocol;
	PrivacyDataPGP *data = NULL;
	
	g_return_val_if_fail(mimeinfo != NULL, FALSE);
	if (mimeinfo->privacy != NULL) {
		data = (PrivacyDataPGP *) mimeinfo->privacy;
		if (data->done_sigtest)
			return data->is_signed;
	}
	
	/* check parent */
	parent = procmime_mimeinfo_parent(mimeinfo);
	if (parent == NULL)
		return FALSE;
	if ((parent->type != MIMETYPE_MULTIPART) ||
	    g_strcasecmp(parent->subtype, "signed"))
		return FALSE;
	protocol = g_hash_table_lookup(parent->parameters, "protocol");
	if ((protocol == NULL) || g_strcasecmp(protocol, "application/pgp-signature"))
		return FALSE;

	/* check if mimeinfo is the first child */
	if (parent->node->children->data != mimeinfo)
		return FALSE;

	/* check signature */
	signature = parent->node->children->next != NULL ? 
	    (MimeInfo *) parent->node->children->next->data : NULL;
	if (signature == NULL)
		return FALSE;
	if ((signature->type != MIMETYPE_APPLICATION) ||
	    g_strcasecmp(signature->subtype, "pgp-signature"))
		return FALSE;

	if (data == NULL) {
		data = pgpmime_new_privacydata();
		mimeinfo->privacy = (PrivacyData *) data;
	}
	data->done_sigtest = TRUE;
	data->is_signed = TRUE;

	if (prefs_common.auto_check_signatures)
		pgpmime_check_signature(mimeinfo);
	
	return TRUE;
}

static gint pgpmime_check_signature(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data;
	MimeInfo *parent, *signature;
	FILE *fp;
	gchar buf[BUFFSIZE];
	gchar *boundary;
	GString *textstr;
	gint boundary_len;
	GpgmeData sigdata, textdata;
	
	g_return_val_if_fail(mimeinfo != NULL, -1);
	g_return_val_if_fail(mimeinfo->privacy != NULL, -1);
	data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	debug_print("Checking PGP/MIME signature\n");
	parent = procmime_mimeinfo_parent(mimeinfo);

	fp = fopen(parent->filename, "rb");
	g_return_val_if_fail(fp != NULL, SIGNATURE_INVALID);
	
	boundary = g_hash_table_lookup(parent->parameters, "boundary");
	boundary_len = strlen(boundary);
	while (fgets(buf, sizeof(buf), fp) != NULL)
		if (IS_BOUNDARY(buf, boundary, boundary_len))
			break;

	textstr = g_string_new("");
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		gchar *buf2;

		if (IS_BOUNDARY(buf, boundary, boundary_len))
			break;
		
		buf2 = canonicalize_str(buf);
		g_string_append(textstr, buf2);
		g_free(buf2);
	}
	g_string_truncate(textstr, textstr->len - 2);
	
	
	gpgme_data_new_from_mem(&textdata, textstr->str, textstr->len, 0);
	signature = (MimeInfo *) mimeinfo->node->next->data;
	gpgme_data_new_from_filepart(&sigdata,
		signature->filename,
		NULL,
		signature->offset,
		signature->length);

	data->sigstatus =
		sgpgme_verify_signature	(data->ctx, sigdata, textdata);
	
	gpgme_data_release(sigdata);
	gpgme_data_release(textdata);
	g_string_free(textstr, TRUE);
	
	return 0;
}

static SignatureStatus pgpmime_get_sig_status(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	g_return_val_if_fail(data != NULL, SIGNATURE_INVALID);

	return sgpgme_sigstat_gpgme_to_privacy(data->sigstatus);
}

static gchar *pgpmime_get_sig_info_short(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	g_return_val_if_fail(data != NULL, g_strdup("Error"));

	return sgpgme_sigstat_info_short(data->ctx, data->sigstatus);
}

static gchar *pgpmime_get_sig_info_full(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	g_return_val_if_fail(data != NULL, g_strdup("Error"));

	return sgpgme_sigstat_info_full(data->ctx, data->sigstatus);
}

static PrivacySystem pgpmime_system = {
	"PGP/Mime",			/* name */

	pgpmime_free_privacydata,	/* free_privacydata */

	pgpmime_is_signed,		/* is_signed(MimeInfo *) */
	pgpmime_check_signature,	/* check_signature(MimeInfo *) */
	pgpmime_get_sig_status,		/* get_sig_status(MimeInfo *) */
	pgpmime_get_sig_info_short,	/* get_sig_info_short(MimeInfo *) */
	pgpmime_get_sig_info_full,	/* get_sig_info_full(MimeInfo *) */

	/* NOT YET */
	NULL,				/* is_encrypted(MimeInfo *) */
	NULL,				/* decrypt(MimeInfo *) */
};

void pgpmime_init()
{
	privacy_register_system(&pgpmime_system);
}

void pgpmime_done()
{
	privacy_unregister_system(&pgpmime_system);
}

#endif /* USE_GPGME */

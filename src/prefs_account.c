/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Hiroyuki Yamamoto and the Claws Mail team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/filesel.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"
#include "prefs_gtk.h"
#include "prefs_account.h"
#include "prefs_common.h"
#include "prefs_customheader.h"
#include "account.h"
#include "mainwindow.h"
#include "manage_window.h"
#include "foldersel.h"
#include "inc.h"
#include "menu.h"
#include "gtkutils.h"
#include "utils.h"
#include "alertpanel.h"
#include "colorlabel.h"
#include "smtp.h"
#include "imap.h"
#include "remotefolder.h"
#include "base64.h"
#include "combobox.h"
#include "setup.h"
#include "quote_fmt.h"

static gboolean cancelled;
static gboolean new_account;
static gboolean create_mailbox;

static PrefsAccount tmp_ac_prefs;

static GtkWidget *sigfile_radiobtn;
static GtkWidget *sigcmd_radiobtn;
static GtkWidget *entry_sigpath;
static GtkWidget *signature_browse_button;
static GtkWidget *signature_edit_button;

static GSList *prefs_pages = NULL;

typedef struct BasicPage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *acname_entry;
	GtkWidget *default_checkbtn;

	GtkWidget *name_entry;
	GtkWidget *addr_entry;
	GtkWidget *org_entry;

	GtkWidget *serv_frame;
	GtkWidget *serv_table;
	gpointer *protocol_optmenu;
	GtkWidget *recvserv_label;
	GtkWidget *smtpserv_label;
	GtkWidget *nntpserv_label;
	GtkWidget *localmbox_label;
	GtkWidget *mailcmd_label;
	GtkWidget *recvserv_entry;
	GtkWidget *smtpserv_entry;
	GtkWidget *nntpserv_entry;
	GtkWidget *nntpauth_checkbtn;
	GtkWidget *nntpauth_onconnect_checkbtn;
	GtkWidget *localmbox_entry;
	GtkWidget *mailcmd_checkbtn;
	GtkWidget *mailcmd_entry;
	GtkWidget *uid_label;
	GtkWidget *pass_label;
	GtkWidget *uid_entry;
	GtkWidget *pass_entry;
} BasicPage;

typedef struct ReceivePage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *pop3_frame;
	GtkWidget *use_apop_checkbtn;
	GtkWidget *rmmail_checkbtn;
	GtkWidget *leave_time_entry;
	GtkWidget *size_limit_checkbtn;
	GtkWidget *size_limit_entry;
	GtkWidget *inbox_label;
	GtkWidget *inbox_entry;
	GtkWidget *inbox_btn;

	GtkWidget *local_frame;
	GtkWidget *local_inbox_label;
	GtkWidget *local_inbox_entry;
	GtkWidget *local_inbox_btn;

	GtkWidget *filter_on_recv_checkbtn;
	GtkWidget *recvatgetall_checkbtn;
	
	GtkWidget *imap_frame;
	GtkWidget *imap_auth_type_optmenu;
	GtkWidget *imapdir_label;
	GtkWidget *imapdir_entry;
	GtkWidget *subsonly_checkbtn;
	GtkWidget *low_bandwidth_checkbtn;

	GtkWidget *frame_maxarticle;
	GtkWidget *maxarticle_label;
	GtkWidget *maxarticle_spinbtn;
	GtkObject *maxarticle_spinbtn_adj;
} ReceivePage;

typedef struct SendPage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *msgid_checkbtn;
	GtkWidget *customhdr_checkbtn;
	GtkWidget *smtp_auth_checkbtn;
	GtkWidget *smtp_auth_type_optmenu;
	GtkWidget *smtp_uid_entry;
	GtkWidget *smtp_pass_entry;
	GtkWidget *pop_bfr_smtp_checkbtn;
	GtkWidget *pop_bfr_smtp_tm_entry;
	GtkWidget *pop_auth_timeout_lbl;
	GtkWidget *pop_auth_minutes_lbl;
} SendPage;

typedef struct ComposePage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *sigfile_radiobtn;
	GtkWidget *entry_sigpath;
	GtkWidget *checkbtn_autosig;
	GtkWidget *entry_sigsep;
	GtkWidget *autocc_checkbtn;
	GtkWidget *autocc_entry;
	GtkWidget *autobcc_checkbtn;
	GtkWidget *autobcc_entry;
	GtkWidget *autoreplyto_checkbtn;
	GtkWidget *autoreplyto_entry;
	GtkWidget *checkbtn_enable_default_dictionary;
	GtkWidget *optmenu_default_dictionary;
	GtkWidget *checkbtn_enable_default_alt_dictionary;
	GtkWidget *optmenu_default_alt_dictionary;
} ComposePage;

typedef struct TemplatesPage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *checkbtn_compose_with_format;
	GtkWidget *compose_subject_format;
	GtkWidget *compose_body_format;
	GtkWidget *checkbtn_reply_with_format;
	GtkWidget *reply_quotemark;
	GtkWidget *reply_body_format;
	GtkWidget *checkbtn_forward_with_format;
	GtkWidget *forward_quotemark;
	GtkWidget *forward_body_format;
} TemplatesPage;

typedef struct PrivacyPage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *default_privacy_system;
	GtkWidget *default_encrypt_checkbtn;
	GtkWidget *default_encrypt_reply_checkbtn;
	GtkWidget *default_sign_checkbtn;
	GtkWidget *save_clear_text_checkbtn;
	GtkWidget *encrypt_to_self_checkbtn;
} PrivacyPage;

typedef struct SSLPage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *pop_frame;
	GtkWidget *pop_nossl_radiobtn;
	GtkWidget *pop_ssltunnel_radiobtn;
	GtkWidget *pop_starttls_radiobtn;

	GtkWidget *imap_frame;
	GtkWidget *imap_nossl_radiobtn;
	GtkWidget *imap_ssltunnel_radiobtn;
	GtkWidget *imap_starttls_radiobtn;

	GtkWidget *nntp_frame;
	GtkWidget *nntp_nossl_radiobtn;
	GtkWidget *nntp_ssltunnel_radiobtn;

	GtkWidget *send_frame;
	GtkWidget *smtp_nossl_radiobtn;
	GtkWidget *smtp_ssltunnel_radiobtn;
	GtkWidget *smtp_starttls_radiobtn;

	GtkWidget *use_nonblocking_ssl_checkbtn;
} SSLPage;

typedef struct AdvancedPage
{
    PrefsPage page;

    GtkWidget *vbox;

	GtkWidget *smtpport_checkbtn;
	GtkWidget *smtpport_entry;
	GtkWidget *popport_hbox;
	GtkWidget *popport_checkbtn;
	GtkWidget *popport_entry;
	GtkWidget *imapport_hbox;
	GtkWidget *imapport_checkbtn;
	GtkWidget *imapport_entry;
	GtkWidget *nntpport_hbox;
	GtkWidget *nntpport_checkbtn;
	GtkWidget *nntpport_entry;
	GtkWidget *domain_checkbtn;
	GtkWidget *domain_entry;
	GtkWidget *crosspost_checkbtn;
 	GtkWidget *crosspost_colormenu;

	GtkWidget *tunnelcmd_checkbtn;
	GtkWidget *tunnelcmd_entry;

	GtkWidget *sent_folder_checkbtn;
	GtkWidget *sent_folder_entry;
	GtkWidget *queue_folder_checkbtn;
	GtkWidget *queue_folder_entry;
	GtkWidget *draft_folder_checkbtn;
	GtkWidget *draft_folder_entry;
	GtkWidget *trash_folder_checkbtn;
	GtkWidget *trash_folder_entry;
} AdvancedPage;

static BasicPage basic_page;
static ReceivePage receive_page;
static SendPage send_page;
static ComposePage compose_page;
static TemplatesPage templates_page;
static PrivacyPage privacy_page;
#if USE_OPENSSL
static SSLPage ssl_page;
#endif
static AdvancedPage advanced_page;

struct BasicProtocol {
	GtkWidget *combobox;
	GtkWidget *label;
	GtkWidget *descrlabel;
	GtkWidget *no_imap_warn_icon;
	GtkWidget *no_imap_warn_label;
};

static char *protocol_names[] = {
	N_("POP3"),
	NULL,		/* APOP, deprecated */
	NULL,		/* RPOP, deprecated */
	N_("IMAP4"),
	N_("News (NNTP)"),
	N_("Local mbox file"),
	N_("None (SMTP only)")
};

static void prefs_account_protocol_set_data_from_optmenu(PrefParam *pparam);
static void prefs_account_protocol_set_optmenu		(PrefParam *pparam);
static void prefs_account_protocol_changed		(GtkComboBox *combobox,
							gpointer data);

static void prefs_account_set_string_from_optmenu	(PrefParam *pparam);
static void prefs_account_set_optmenu_from_string	(PrefParam *pparam);

static void prefs_account_imap_auth_type_set_data_from_optmenu
							(PrefParam *pparam);
static void prefs_account_imap_auth_type_set_optmenu	(PrefParam *pparam);
static void prefs_account_smtp_auth_type_set_data_from_optmenu
							(PrefParam *pparam);
static void prefs_account_smtp_auth_type_set_optmenu	(PrefParam *pparam);

static void prefs_account_enum_set_data_from_radiobtn	(PrefParam *pparam);
static void prefs_account_enum_set_radiobtn		(PrefParam *pparam);

static void crosspost_color_toggled(void);
static void prefs_account_crosspost_set_data_from_colormenu(PrefParam *pparam);
static void prefs_account_crosspost_set_colormenu(PrefParam *pparam);

static void prefs_account_nntpauth_toggled(GtkToggleButton *button,
					   gpointer user_data);
static void prefs_account_mailcmd_toggled(GtkToggleButton *button,
					  gpointer user_data);

#if USE_ASPELL
static void prefs_account_compose_default_dictionary_set_string_from_optmenu
							(PrefParam *pparam);
static void prefs_account_compose_default_dictionary_set_optmenu_from_string
							(PrefParam *pparam);
#endif

static gchar *privacy_prefs;

static PrefParam basic_param[] = {
	{"account_name", NULL, &tmp_ac_prefs.account_name, P_STRING,
	 &basic_page.acname_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"is_default", "FALSE", &tmp_ac_prefs.is_default, P_BOOL,
	 &basic_page.default_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"name", NULL, &tmp_ac_prefs.name, P_STRING,
	 &basic_page.name_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"address", NULL, &tmp_ac_prefs.address, P_STRING,
	 &basic_page.addr_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"organization", NULL, &tmp_ac_prefs.organization, P_STRING,
	 &basic_page.org_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"protocol", NULL, &tmp_ac_prefs.protocol, P_ENUM,
	 (GtkWidget **)&basic_page.protocol_optmenu,
	 prefs_account_protocol_set_data_from_optmenu,
	 prefs_account_protocol_set_optmenu},

	{"receive_server", NULL, &tmp_ac_prefs.recv_server, P_STRING,
	 &basic_page.recvserv_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"smtp_server", NULL, &tmp_ac_prefs.smtp_server, P_STRING,
	 &basic_page.smtpserv_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"nntp_server", NULL, &tmp_ac_prefs.nntp_server, P_STRING,
	 &basic_page.nntpserv_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"local_mbox", "/var/mail", &tmp_ac_prefs.local_mbox, P_STRING,
	 &basic_page.localmbox_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"use_mail_command", "FALSE", &tmp_ac_prefs.use_mail_command, P_BOOL,
	 &basic_page.mailcmd_checkbtn, prefs_set_data_from_toggle, prefs_set_toggle},

	{"mail_command", DEFAULT_SENDMAIL_CMD, &tmp_ac_prefs.mail_command, P_STRING,
	 &basic_page.mailcmd_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"use_nntp_auth", "FALSE", &tmp_ac_prefs.use_nntp_auth, P_BOOL,
	 &basic_page.nntpauth_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	
	{"use_nntp_auth_onconnect", "FALSE", &tmp_ac_prefs.use_nntp_auth_onconnect, P_BOOL,
	 &basic_page.nntpauth_onconnect_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"user_id", "ENV_USER", &tmp_ac_prefs.userid, P_STRING,
	 &basic_page.uid_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"password", NULL, &tmp_ac_prefs.passwd, P_PASSWORD,
	 &basic_page.pass_entry, prefs_set_data_from_entry, prefs_set_entry},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam receive_param[] = {
	{"use_apop_auth", "FALSE", &tmp_ac_prefs.use_apop_auth, P_BOOL,
	 &receive_page.use_apop_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"remove_mail", "TRUE", &tmp_ac_prefs.rmmail, P_BOOL,
	 &receive_page.rmmail_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

#ifndef MAEMO
	{"message_leave_time", "7", &tmp_ac_prefs.msg_leave_time, P_INT,
	 &receive_page.leave_time_entry,
	 prefs_set_data_from_entry, prefs_set_entry},
#else
	{"message_leave_time", "30", &tmp_ac_prefs.msg_leave_time, P_INT,
	 &receive_page.leave_time_entry,
	 prefs_set_data_from_entry, prefs_set_entry},
#endif

	{"enable_size_limit", "FALSE", &tmp_ac_prefs.enable_size_limit, P_BOOL,
	 &receive_page.size_limit_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"size_limit", "1024", &tmp_ac_prefs.size_limit, P_INT,
	 &receive_page.size_limit_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"filter_on_receive", "TRUE", &tmp_ac_prefs.filter_on_recv, P_BOOL,
	 &receive_page.filter_on_recv_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"imap_auth_method", "0", &tmp_ac_prefs.imap_auth_type, P_ENUM,
	 &receive_page.imap_auth_type_optmenu,
	 prefs_account_imap_auth_type_set_data_from_optmenu,
	 prefs_account_imap_auth_type_set_optmenu},

	{"receive_at_get_all", "TRUE", &tmp_ac_prefs.recv_at_getall, P_BOOL,
	 &receive_page.recvatgetall_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"max_news_articles", "300", &tmp_ac_prefs.max_articles, P_INT,
	 &receive_page.maxarticle_spinbtn,
	 prefs_set_data_from_spinbtn, prefs_set_spinbtn},

	{"inbox", "#mh/Mailbox/inbox", &tmp_ac_prefs.inbox, P_STRING,
	 &receive_page.inbox_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"local_inbox", "#mh/Mailbox/inbox", &tmp_ac_prefs.local_inbox, P_STRING,
	 &receive_page.local_inbox_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"imap_directory", NULL, &tmp_ac_prefs.imap_dir, P_STRING,
	 &receive_page.imapdir_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"imap_subsonly", "TRUE", &tmp_ac_prefs.imap_subsonly, P_BOOL,
	 &receive_page.subsonly_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"low_bandwidth", "TRUE", &tmp_ac_prefs.low_bandwidth, P_BOOL,
	 &receive_page.low_bandwidth_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam send_param[] = {
	{"generate_msgid", "TRUE", &tmp_ac_prefs.gen_msgid, P_BOOL,
	 &send_page.msgid_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"add_custom_header", "FALSE", &tmp_ac_prefs.add_customhdr, P_BOOL,
	 &send_page.customhdr_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"use_smtp_auth", "FALSE", &tmp_ac_prefs.use_smtp_auth, P_BOOL,
	 &send_page.smtp_auth_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"smtp_auth_method", "0", &tmp_ac_prefs.smtp_auth_type, P_ENUM,
	 &send_page.smtp_auth_type_optmenu,
	 prefs_account_smtp_auth_type_set_data_from_optmenu,
	 prefs_account_smtp_auth_type_set_optmenu},

	{"smtp_user_id", NULL, &tmp_ac_prefs.smtp_userid, P_STRING,
	 &send_page.smtp_uid_entry, prefs_set_data_from_entry, prefs_set_entry},
	{"smtp_password", NULL, &tmp_ac_prefs.smtp_passwd, P_PASSWORD,
	 &send_page.smtp_pass_entry, prefs_set_data_from_entry, prefs_set_entry},

	{"pop_before_smtp", "FALSE", &tmp_ac_prefs.pop_before_smtp, P_BOOL,
	 &send_page.pop_bfr_smtp_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"pop_before_smtp_timeout", "5", &tmp_ac_prefs.pop_before_smtp_timeout, P_INT,
	 &send_page.pop_bfr_smtp_tm_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam compose_param[] = {
	{"signature_type", "0", &tmp_ac_prefs.sig_type, P_ENUM,
	 &compose_page.sigfile_radiobtn,
	 prefs_account_enum_set_data_from_radiobtn,
	 prefs_account_enum_set_radiobtn},
	{"signature_path", "~" G_DIR_SEPARATOR_S DEFAULT_SIGNATURE,
	 &tmp_ac_prefs.sig_path, P_STRING, &compose_page.entry_sigpath,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"auto_signature", "TRUE", &tmp_ac_prefs.auto_sig, P_BOOL,
	 &compose_page.checkbtn_autosig,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	 
	{"signature_separator", "-- ", &tmp_ac_prefs.sig_sep, P_STRING,
	 &compose_page.entry_sigsep, 
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_autocc", "FALSE", &tmp_ac_prefs.set_autocc, P_BOOL,
	 &compose_page.autocc_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"auto_cc", NULL, &tmp_ac_prefs.auto_cc, P_STRING,
	 &compose_page.autocc_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_autobcc", "FALSE", &tmp_ac_prefs.set_autobcc, P_BOOL,
	 &compose_page.autobcc_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"auto_bcc", NULL, &tmp_ac_prefs.auto_bcc, P_STRING,
	 &compose_page.autobcc_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_autoreplyto", "FALSE", &tmp_ac_prefs.set_autoreplyto, P_BOOL,
	 &compose_page.autoreplyto_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"auto_replyto", NULL, &tmp_ac_prefs.auto_replyto, P_STRING,
	 &compose_page.autoreplyto_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

#if USE_ASPELL
	{"enable_default_dictionary", "", &tmp_ac_prefs.enable_default_dictionary, P_BOOL,
	 &compose_page.checkbtn_enable_default_dictionary,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"default_dictionary", NULL, &tmp_ac_prefs.default_dictionary, P_STRING,
	 &compose_page.optmenu_default_dictionary,
	 prefs_account_compose_default_dictionary_set_string_from_optmenu,
	 prefs_account_compose_default_dictionary_set_optmenu_from_string},

	{"enable_default_alt_dictionary", "", &tmp_ac_prefs.enable_default_alt_dictionary, P_BOOL,
	 &compose_page.checkbtn_enable_default_alt_dictionary,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"default_alt_dictionary", NULL, &tmp_ac_prefs.default_alt_dictionary, P_STRING,
	 &compose_page.optmenu_default_alt_dictionary,
	 prefs_account_compose_default_dictionary_set_string_from_optmenu,
	 prefs_account_compose_default_dictionary_set_optmenu_from_string},
#else
	{"enable_default_dictionary", "", &tmp_ac_prefs.enable_default_dictionary, P_BOOL,
	 NULL, NULL, NULL},

	{"default_dictionary", NULL, &tmp_ac_prefs.default_dictionary, P_STRING,
	 NULL, NULL, NULL},

	{"enable_default_alt_dictionary", "", &tmp_ac_prefs.enable_default_alt_dictionary, P_BOOL,
	 NULL, NULL, NULL},

	{"default_alt_dictionary", NULL, &tmp_ac_prefs.default_alt_dictionary, P_STRING,
	 NULL, NULL, NULL},
#endif	 

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam templates_param[] = {
	{"compose_with_format", "FALSE", &tmp_ac_prefs.compose_with_format, P_BOOL,
	 &templates_page.checkbtn_compose_with_format,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"compose_subject_format", NULL, &tmp_ac_prefs.compose_subject_format, P_STRING,
	 &templates_page.compose_subject_format,
	 prefs_set_escaped_data_from_entry, prefs_set_entry_from_escaped},

	{"compose_body_format", NULL, &tmp_ac_prefs.compose_body_format, P_STRING,
	 &templates_page.compose_body_format,
	 prefs_set_escaped_data_from_text, prefs_set_text_from_escaped},

	{"reply_with_format", "FALSE", &tmp_ac_prefs.reply_with_format, P_BOOL,
	 &templates_page.checkbtn_reply_with_format,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"reply_quotemark", NULL, &tmp_ac_prefs.reply_quotemark, P_STRING,
	 &templates_page.reply_quotemark,
	 prefs_set_data_from_entry, prefs_set_entry_from_escaped},

	{"reply_body_format", NULL, &tmp_ac_prefs.reply_body_format, P_STRING,
	 &templates_page.reply_body_format,
	 prefs_set_escaped_data_from_text, prefs_set_text_from_escaped},

	{"forward_with_format", "FALSE", &tmp_ac_prefs.forward_with_format, P_BOOL,
	 &templates_page.checkbtn_forward_with_format,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"forward_quotemark", NULL, &tmp_ac_prefs.forward_quotemark, P_STRING,
	 &templates_page.forward_quotemark,
	 prefs_set_data_from_entry, prefs_set_entry_from_escaped},

	{"forward_body_format", NULL, &tmp_ac_prefs.forward_body_format, P_STRING,
	 &templates_page.forward_body_format,
	 prefs_set_escaped_data_from_text, prefs_set_text_from_escaped},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam privacy_param[] = {
	{"default_privacy_system", "", &tmp_ac_prefs.default_privacy_system, P_STRING,
	 &privacy_page.default_privacy_system,
	 prefs_account_set_string_from_optmenu, prefs_account_set_optmenu_from_string},

	{"default_encrypt", "FALSE", &tmp_ac_prefs.default_encrypt, P_BOOL,
	 &privacy_page.default_encrypt_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"default_encrypt_reply", "TRUE", &tmp_ac_prefs.default_encrypt_reply, P_BOOL,
	 &privacy_page.default_encrypt_reply_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"default_sign", "FALSE", &tmp_ac_prefs.default_sign, P_BOOL,
	 &privacy_page.default_sign_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"save_clear_text", "FALSE", &tmp_ac_prefs.save_encrypted_as_clear_text, P_BOOL,
	 &privacy_page.save_clear_text_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"encrypt_to_self", "FALSE", &tmp_ac_prefs.encrypt_to_self, P_BOOL,
	 &privacy_page.encrypt_to_self_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"privacy_prefs", "", &privacy_prefs, P_STRING,
	 NULL, NULL, NULL},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam ssl_param[] = {
#if USE_OPENSSL
	{"ssl_pop", "0", &tmp_ac_prefs.ssl_pop, P_ENUM,
	 &ssl_page.pop_nossl_radiobtn,
	 prefs_account_enum_set_data_from_radiobtn,
	 prefs_account_enum_set_radiobtn},

	{"ssl_imap", "0", &tmp_ac_prefs.ssl_imap, P_ENUM,
	 &ssl_page.imap_nossl_radiobtn,
	 prefs_account_enum_set_data_from_radiobtn,
	 prefs_account_enum_set_radiobtn},

	{"ssl_nntp", "0", &tmp_ac_prefs.ssl_nntp, P_ENUM,
	 &ssl_page.nntp_nossl_radiobtn,
	 prefs_account_enum_set_data_from_radiobtn,
	 prefs_account_enum_set_radiobtn},

	{"ssl_smtp", "0", &tmp_ac_prefs.ssl_smtp, P_ENUM,
	 &ssl_page.smtp_nossl_radiobtn,
	 prefs_account_enum_set_data_from_radiobtn,
	 prefs_account_enum_set_radiobtn},

	{"use_nonblocking_ssl", "1", &tmp_ac_prefs.use_nonblocking_ssl, P_BOOL,
	 &ssl_page.use_nonblocking_ssl_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
#else
	{"ssl_pop", "0", &tmp_ac_prefs.ssl_pop, P_ENUM,
	 NULL, NULL, NULL},

	{"ssl_imap", "0", &tmp_ac_prefs.ssl_imap, P_ENUM,
	 NULL, NULL, NULL},

	{"ssl_nntp", "0", &tmp_ac_prefs.ssl_nntp, P_ENUM,
	 NULL, NULL, NULL},

	{"ssl_smtp", "0", &tmp_ac_prefs.ssl_smtp, P_ENUM,
	 NULL, NULL, NULL},

	{"use_nonblocking_ssl", "1", &tmp_ac_prefs.use_nonblocking_ssl, P_BOOL,
	 NULL, NULL, NULL},
#endif /* USE_OPENSSL */

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static PrefParam advanced_param[] = {
	{"set_smtpport", "FALSE", &tmp_ac_prefs.set_smtpport, P_BOOL,
	 &advanced_page.smtpport_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"smtp_port", "25", &tmp_ac_prefs.smtpport, P_USHORT,
	 &advanced_page.smtpport_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_popport", "FALSE", &tmp_ac_prefs.set_popport, P_BOOL,
	 &advanced_page.popport_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"pop_port", "110", &tmp_ac_prefs.popport, P_USHORT,
	 &advanced_page.popport_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_imapport", "FALSE", &tmp_ac_prefs.set_imapport, P_BOOL,
	 &advanced_page.imapport_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"imap_port", "143", &tmp_ac_prefs.imapport, P_USHORT,
	 &advanced_page.imapport_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_nntpport", "FALSE", &tmp_ac_prefs.set_nntpport, P_BOOL,
	 &advanced_page.nntpport_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"nntp_port", "119", &tmp_ac_prefs.nntpport, P_USHORT,
	 &advanced_page.nntpport_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_domain", "FALSE", &tmp_ac_prefs.set_domain, P_BOOL,
	 &advanced_page.domain_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"domain", NULL, &tmp_ac_prefs.domain, P_STRING,
	 &advanced_page.domain_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_tunnelcmd", "FALSE", &tmp_ac_prefs.set_tunnelcmd, P_BOOL,
	 &advanced_page.tunnelcmd_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"tunnelcmd", NULL, &tmp_ac_prefs.tunnelcmd, P_STRING,
	 &advanced_page.tunnelcmd_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"mark_crosspost_read", "FALSE", &tmp_ac_prefs.mark_crosspost_read, P_BOOL,
	 &advanced_page.crosspost_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},

	{"crosspost_color", NULL, &tmp_ac_prefs.crosspost_col, P_ENUM,
	 &advanced_page.crosspost_colormenu,
	 prefs_account_crosspost_set_data_from_colormenu,
	 prefs_account_crosspost_set_colormenu},

	{"set_sent_folder", "FALSE", &tmp_ac_prefs.set_sent_folder, P_BOOL,
	 &advanced_page.sent_folder_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"sent_folder", NULL, &tmp_ac_prefs.sent_folder, P_STRING,
	 &advanced_page.sent_folder_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_queue_folder", "FALSE", &tmp_ac_prefs.set_queue_folder, P_BOOL,
	 &advanced_page.queue_folder_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"queue_folder", NULL, &tmp_ac_prefs.queue_folder, P_STRING,
	 &advanced_page.queue_folder_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_draft_folder", "FALSE", &tmp_ac_prefs.set_draft_folder, P_BOOL,
	 &advanced_page.draft_folder_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"draft_folder", NULL, &tmp_ac_prefs.draft_folder, P_STRING,
	 &advanced_page.draft_folder_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{"set_trash_folder", "FALSE", &tmp_ac_prefs.set_trash_folder, P_BOOL,
	 &advanced_page.trash_folder_checkbtn,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"trash_folder", NULL, &tmp_ac_prefs.trash_folder, P_STRING,
	 &advanced_page.trash_folder_entry,
	 prefs_set_data_from_entry, prefs_set_entry},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static gint prefs_account_get_new_id		(void);

static void prefs_account_select_folder_cb	(GtkWidget	*widget,
						 gpointer	 data);

static void prefs_account_sigfile_radiobtn_cb	(GtkWidget	*widget,
						 gpointer	 data);

static void prefs_account_sigcmd_radiobtn_cb	(GtkWidget	*widget,
						 gpointer	 data);

static void prefs_account_signature_browse_cb	(GtkWidget	*widget,
						 gpointer	 data);

static void prefs_account_signature_edit_cb	(GtkWidget	*widget,
						 gpointer	 data);

static void pop_bfr_smtp_tm_set_sens		(GtkWidget	*widget,
						 gpointer	 data);

static void prefs_account_edit_custom_header	(void);

static void privacy_system_activated(GtkMenuItem *menuitem)
{
	const gchar* system_id;
	gboolean privacy_enabled = FALSE;
	
	system_id = g_object_get_data(G_OBJECT(menuitem), MENU_VAL_ID);
	
	privacy_enabled = strcmp(system_id, "");

	if (g_object_get_data(G_OBJECT(menuitem), MENU_VAL_DATA) == NULL ||
	    GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), MENU_VAL_DATA)) == FALSE)
		privacy_enabled = FALSE;

	gtk_widget_set_sensitive (privacy_page.default_encrypt_checkbtn, privacy_enabled);
	gtk_widget_set_sensitive (privacy_page.default_encrypt_reply_checkbtn, privacy_enabled);
	gtk_widget_set_sensitive (privacy_page.default_sign_checkbtn, privacy_enabled);
	gtk_widget_set_sensitive (privacy_page.encrypt_to_self_checkbtn, privacy_enabled);
	gtk_widget_set_sensitive (privacy_page.save_clear_text_checkbtn, 
		privacy_enabled && !gtk_toggle_button_get_active(
					GTK_TOGGLE_BUTTON(privacy_page.encrypt_to_self_checkbtn)));
}

static void update_privacy_system_menu() {
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList *system_ids, *cur;

	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label(_("None"));
	gtk_widget_show(menuitem);
	g_object_set_data(G_OBJECT(menuitem), MENU_VAL_ID, "");
	g_object_set_data(G_OBJECT(menuitem), MENU_VAL_DATA, GINT_TO_POINTER(FALSE));
	gtk_menu_append(GTK_MENU(menu), menuitem);

	g_signal_connect(G_OBJECT(menuitem), "activate",
			 G_CALLBACK(privacy_system_activated),
			 NULL);

	system_ids = privacy_get_system_ids();
	for (cur = system_ids; cur != NULL; cur = g_slist_next(cur)) {
		gchar *id = (gchar *) cur->data;
		const gchar *name;
		
		name = privacy_system_get_name(id);
		menuitem = gtk_menu_item_new_with_label(name);
		gtk_widget_show(menuitem);
		g_object_set_data_full(G_OBJECT(menuitem), MENU_VAL_ID, id, g_free);
		g_object_set_data(G_OBJECT(menuitem), MENU_VAL_DATA, GINT_TO_POINTER(TRUE));

		gtk_menu_append(GTK_MENU(menu), menuitem);

		g_signal_connect(G_OBJECT(menuitem), "activate",
				 G_CALLBACK(privacy_system_activated),
				 NULL);

		
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(privacy_page.default_privacy_system), menu);
}

#define TABLE_YPAD 2

static void basic_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	BasicPage *page = (BasicPage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *acname_entry;
	GtkWidget *default_checkbtn;
	GtkWidget *frame1;
	GtkWidget *table1;
	GtkWidget *name_entry;
	GtkWidget *addr_entry;
	GtkWidget *org_entry;

	GtkWidget *serv_frame;
	GtkWidget *vbox2;
	GtkWidget *optmenubox;
	GtkWidget *optmenu;
	GtkWidget *optlabel;
	GtkWidget *no_imap_warn_icon;
	GtkWidget *no_imap_warn_label;
	GtkWidget *serv_table;
	GtkWidget *recvserv_label;
	GtkWidget *smtpserv_label;
	GtkWidget *nntpserv_label;
	GtkWidget *localmbox_label;
	GtkWidget *mailcmd_label;
	GtkWidget *recvserv_entry;
	GtkWidget *smtpserv_entry;
	GtkWidget *nntpserv_entry;
	GtkWidget *nntpauth_checkbtn;
	GtkWidget *nntpauth_onconnect_checkbtn;
	GtkWidget *localmbox_entry;
	GtkWidget *mailcmd_checkbtn;
	GtkWidget *mailcmd_entry;
	GtkWidget *uid_label;
	GtkWidget *pass_label;
	GtkWidget *uid_entry;
	GtkWidget *pass_entry;
	GtkListStore *menu;
	GtkTreeIter iter;

	struct BasicProtocol *protocol_optmenu;
	gint i;

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Name of account"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	acname_entry = gtk_entry_new ();
	gtk_widget_show (acname_entry);
	gtk_widget_set_size_request (acname_entry, DEFAULT_ENTRY_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (hbox), acname_entry, TRUE, TRUE, 0);

	default_checkbtn = gtk_check_button_new_with_label (_("Set as default"));
	gtk_widget_show (default_checkbtn);
#ifndef MAEMO
	gtk_box_pack_end (GTK_BOX (hbox), default_checkbtn, TRUE, FALSE, 0);
#else
	gtk_box_pack_start (GTK_BOX (vbox1), default_checkbtn, FALSE, FALSE, 0);
	
#endif
	PACK_FRAME (vbox1, frame1, _("Personal information"));

	table1 = gtk_table_new (3, 2, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);
	gtk_container_set_border_width (GTK_CONTAINER (table1), 8);
	gtk_table_set_row_spacings (GTK_TABLE (table1), VSPACING_NARROW);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 8);

	label = gtk_label_new (_("Full name"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);

	label = gtk_label_new (_("Mail address"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 1, 2,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);

	label = gtk_label_new (_("Organization"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 2, 3,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);

	name_entry = gtk_entry_new ();
	gtk_widget_show (name_entry);
	gtk_table_attach (GTK_TABLE (table1), name_entry, 1, 2, 0, 1,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	addr_entry = gtk_entry_new ();
	gtk_widget_show (addr_entry);
	gtk_table_attach (GTK_TABLE (table1), addr_entry, 1, 2, 1, 2,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	org_entry = gtk_entry_new ();
	gtk_widget_show (org_entry);
	gtk_table_attach (GTK_TABLE (table1), org_entry, 1, 2, 2, 3,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	vbox2 = gtkut_get_options_frame(vbox1, &serv_frame, _("Server information"));

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Protocol"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* Create GtkHBox for protocol combobox and label */
	optmenubox = gtk_hbox_new(FALSE, 20);
	gtk_widget_show(optmenubox);
	gtk_box_pack_start (GTK_BOX (hbox), optmenubox, FALSE, FALSE, 0);

	/* Create and populate the combobox */
	optmenu = gtkut_sc_combobox_create(NULL, FALSE);
	gtk_box_pack_start(GTK_BOX (optmenubox), optmenu, FALSE, FALSE, 0);

	menu = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(optmenu)));
	for( i = 0; i < NUM_RECV_PROTOCOLS; i++ )
		if( protocol_names[i] != NULL )
			COMBOBOX_ADD (menu, _(protocol_names[i]), i);

	g_signal_connect(G_OBJECT(optmenu), "changed",
			G_CALLBACK(prefs_account_protocol_changed), NULL);

	/* Create protocol label, empty for now */
	optlabel = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(optlabel), TRUE);
	gtk_label_set_justify(GTK_LABEL(optlabel), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX (optmenubox), optlabel, FALSE, FALSE, 0);

	no_imap_warn_icon = gtk_image_new_from_stock
                        (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR);
	no_imap_warn_label = gtk_label_new(_("<span weight=\"bold\">Warning: this version of Claws Mail\n"
			  "has been built without IMAP support.</span>"));
	gtk_label_set_use_markup(GTK_LABEL(no_imap_warn_label), TRUE);

	gtk_box_pack_start(GTK_BOX (optmenubox), no_imap_warn_icon, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (optmenubox), no_imap_warn_label, FALSE, FALSE, 0);
	/* Set up a struct to store pointers to necessary widgets */
	protocol_optmenu = g_new(struct BasicProtocol, 1);
	protocol_optmenu->combobox = optmenu;
	protocol_optmenu->label = optlabel;
	protocol_optmenu->descrlabel = label;
	protocol_optmenu->no_imap_warn_icon = no_imap_warn_icon;
	protocol_optmenu->no_imap_warn_label = no_imap_warn_label;

	serv_table = gtk_table_new (6, 4, FALSE);
	gtk_widget_show (serv_table);
	gtk_box_pack_start (GTK_BOX (vbox2), serv_table, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (serv_table), VSPACING_NARROW);
	gtk_table_set_row_spacing (GTK_TABLE (serv_table), 3, 0);
	gtk_table_set_col_spacings (GTK_TABLE (serv_table), 8);

	nntpserv_entry = gtk_entry_new ();
	gtk_widget_show (nntpserv_entry);
	gtk_table_attach (GTK_TABLE (serv_table), nntpserv_entry, 1, 4, 0, 1,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
/*  	gtk_table_set_row_spacing (GTK_TABLE (serv_table), 0, 0); */

	nntpauth_checkbtn = gtk_check_button_new_with_label
		(_("This server requires authentication"));
	gtk_widget_show (nntpauth_checkbtn);
	
	gtk_table_attach (GTK_TABLE (serv_table), nntpauth_checkbtn, 0, 2, 6, 7,
			  GTK_FILL, 0, 0, 0);

	nntpauth_onconnect_checkbtn = gtk_check_button_new_with_label
		(_("Authenticate on connect"));
	gtk_widget_show (nntpauth_onconnect_checkbtn);

	gtk_table_attach (GTK_TABLE (serv_table), nntpauth_onconnect_checkbtn, 2, 4, 6, 7,
			  GTK_FILL, 0, 0, 0);

	recvserv_entry = gtk_entry_new ();
	gtk_widget_show (recvserv_entry);
	gtk_table_attach (GTK_TABLE (serv_table), recvserv_entry, 1, 4, 2, 3,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	localmbox_entry = gtk_entry_new ();
	gtk_widget_show (localmbox_entry);
	gtk_table_attach (GTK_TABLE (serv_table), localmbox_entry, 1, 4, 3, 4,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	smtpserv_entry = gtk_entry_new ();
	gtk_widget_show (smtpserv_entry);
	gtk_table_attach (GTK_TABLE (serv_table), smtpserv_entry, 1, 4, 4, 5,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	mailcmd_entry = gtk_entry_new ();
	gtk_widget_show (mailcmd_entry);
	gtk_table_attach (GTK_TABLE (serv_table), mailcmd_entry, 1, 4, 6, 7,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	uid_entry = gtk_entry_new ();
	gtk_widget_show (uid_entry);
	gtk_widget_set_size_request (uid_entry, DEFAULT_ENTRY_WIDTH, -1);
	pass_entry = gtk_entry_new ();
	gtk_widget_show (pass_entry);
	gtk_widget_set_size_request (pass_entry, DEFAULT_ENTRY_WIDTH, -1);
#ifndef MAEMO
	gtk_table_attach (GTK_TABLE (serv_table), uid_entry, 1, 2, 7, 8,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	gtk_table_attach (GTK_TABLE (serv_table), pass_entry, 3, 4, 7, 8,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
#else
	gtk_table_attach (GTK_TABLE (serv_table), uid_entry, 1, 4, 7, 8,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach (GTK_TABLE (serv_table), pass_entry, 1, 4, 8, 9,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
#endif
	gtk_entry_set_visibility (GTK_ENTRY (pass_entry), FALSE);

	nntpserv_label = gtk_label_new (_("News server"));
	gtk_widget_show (nntpserv_label);
	gtk_table_attach (GTK_TABLE (serv_table), nntpserv_label, 0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (nntpserv_label), 1, 0.5);

	recvserv_label = gtk_label_new (_("Server for receiving"));
	gtk_widget_show (recvserv_label);
	gtk_table_attach (GTK_TABLE (serv_table), recvserv_label, 0, 1, 2, 3,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (recvserv_label), 1, 0.5);

	localmbox_label = gtk_label_new (_("Local mailbox"));
	gtk_widget_show (localmbox_label);
	gtk_table_attach (GTK_TABLE (serv_table), localmbox_label, 0, 1, 3, 4,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (localmbox_label), 1, 0.5);
/*  	gtk_table_set_row_spacing (GTK_TABLE (serv_table), 2, 0); */

	smtpserv_label = gtk_label_new (_("SMTP server (send)"));
	gtk_widget_show (smtpserv_label);
	gtk_table_attach (GTK_TABLE (serv_table), smtpserv_label, 0, 1, 4, 5,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (smtpserv_label), 1, 0.5);
/*  	gtk_table_set_row_spacing (GTK_TABLE (serv_table), 2, 0); */

	mailcmd_checkbtn = gtk_check_button_new_with_label
		(_("Use mail command rather than SMTP server"));
	gtk_widget_show (mailcmd_checkbtn);
	gtk_table_attach (GTK_TABLE (serv_table), mailcmd_checkbtn, 0, 4, 5, 6,
			  GTK_EXPAND | GTK_FILL,
			  0, 0, TABLE_YPAD);
	g_signal_connect(G_OBJECT(mailcmd_checkbtn), "toggled",
			 G_CALLBACK(prefs_account_mailcmd_toggled),
			 NULL);

	mailcmd_label = gtk_label_new (_("command to send mails"));
	gtk_widget_show (mailcmd_label);
	gtk_table_attach (GTK_TABLE (serv_table), mailcmd_label, 0, 1, 6, 7,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (mailcmd_label), 1, 0.5);
/*  	gtk_table_set_row_spacing (GTK_TABLE (serv_table), 2, 0); */

	uid_label = gtk_label_new (_("User ID"));
	gtk_widget_show (uid_label);
	gtk_table_attach (GTK_TABLE (serv_table), uid_label, 0, 1, 7, 8,
			  GTK_FILL, 0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (uid_label), 1, 0.5);

	pass_label = gtk_label_new (_("Password"));
	gtk_widget_show (pass_label);
#ifndef MAEMO
	gtk_table_attach (GTK_TABLE (serv_table), pass_label, 2, 3, 7, 8,
			  0, 0, 0, 0);
#else
	gtk_misc_set_alignment (GTK_MISC (pass_label), 1, 0.5);
	gtk_table_attach (GTK_TABLE (serv_table), pass_label, 0, 1, 8, 9,
			  GTK_FILL, 0, 0, 0);
#endif
	SET_TOGGLE_SENSITIVITY (nntpauth_checkbtn, uid_label);
	SET_TOGGLE_SENSITIVITY (nntpauth_checkbtn, pass_label);
	SET_TOGGLE_SENSITIVITY (nntpauth_checkbtn, uid_entry);
	SET_TOGGLE_SENSITIVITY (nntpauth_checkbtn, pass_entry);
	SET_TOGGLE_SENSITIVITY (nntpauth_checkbtn, nntpauth_onconnect_checkbtn);

	page->acname_entry   = acname_entry;
	page->default_checkbtn = default_checkbtn;

	page->name_entry = name_entry;
	page->addr_entry = addr_entry;
	page->org_entry  = org_entry;

	page->serv_frame       = serv_frame;
	page->serv_table       = serv_table;
	page->protocol_optmenu = (gpointer)protocol_optmenu;
	page->recvserv_label   = recvserv_label;
	page->recvserv_entry   = recvserv_entry;
	page->smtpserv_label   = smtpserv_label;
	page->smtpserv_entry   = smtpserv_entry;
	page->nntpserv_label   = nntpserv_label;
	page->nntpserv_entry   = nntpserv_entry;
	page->nntpauth_checkbtn  = nntpauth_checkbtn;
	page->nntpauth_onconnect_checkbtn  = nntpauth_onconnect_checkbtn;
	page->localmbox_label   = localmbox_label;
	page->localmbox_entry   = localmbox_entry;
	page->mailcmd_checkbtn   = mailcmd_checkbtn;
	page->mailcmd_label   = mailcmd_label;
	page->mailcmd_entry   = mailcmd_entry;
	page->uid_label        = uid_label;
	page->pass_label       = pass_label;
	page->uid_entry        = uid_entry;
	page->pass_entry       = pass_entry;

	create_mailbox = FALSE;
	if (new_account) {
		PrefsAccount *def_ac;
		gchar *buf;

		prefs_set_dialog_to_default(basic_param);
		buf = g_strdup_printf(_("Account%d"), ac_prefs->account_id);
		gtk_entry_set_text(GTK_ENTRY(basic_page.acname_entry), buf);
		g_free(buf);
		def_ac = account_get_default();
		if (def_ac) {
			FolderItem *item = folder_get_default_inbox_for_class(F_MH);
			gtk_entry_set_text(GTK_ENTRY(basic_page.name_entry),
					   def_ac->name ? def_ac->name : "");
			gtk_entry_set_text(GTK_ENTRY(basic_page.addr_entry),
					   def_ac->address ? def_ac->address : "");
			gtk_entry_set_text(GTK_ENTRY(basic_page.org_entry),
					   def_ac->organization ? def_ac->organization : "");
			if (!item) {
				item = folder_get_default_inbox();
			}
			if (item) {
				gchar *id = folder_item_get_identifier(item);
				gtk_entry_set_text(GTK_ENTRY(receive_page.inbox_entry),
					id);
				gtk_entry_set_text(GTK_ENTRY(receive_page.local_inbox_entry),
					id);
				g_free(id);
			} else {
				create_mailbox = TRUE;
			}
		}
	} else
		prefs_set_dialog(basic_param);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}

static void receive_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	ReceivePage *page = (ReceivePage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;
	GtkWidget *frame1;
	GtkWidget *vbox2;
	GtkWidget *use_apop_checkbtn;
	GtkWidget *rmmail_checkbtn;
	GtkWidget *hbox_spc;
	GtkTooltips *leave_time_tooltip;
	GtkWidget *leave_time_label;
	GtkWidget *leave_time_entry;
	GtkWidget *hbox1;
	GtkWidget *size_limit_checkbtn;
	GtkWidget *size_limit_entry;
	GtkTooltips *size_limit_tooltip;
	GtkWidget *label;
	GtkWidget *filter_on_recv_checkbtn;
	GtkWidget *vbox3;
	GtkWidget *inbox_label;
	GtkWidget *inbox_entry;
	GtkWidget *inbox_btn;
	GtkTooltips *inbox_tooltip;
	GtkWidget *imap_frame;
 	GtkWidget *imapdir_label;
	GtkWidget *imapdir_entry;
	GtkWidget *subsonly_checkbtn;
	GtkWidget *low_bandwidth_checkbtn;
	GtkWidget *local_frame;
	GtkWidget *local_vbox;
	GtkWidget *local_hbox;
	GtkWidget *local_inbox_label;
	GtkWidget *local_inbox_entry;
	GtkWidget *local_inbox_btn;

	GtkWidget *optmenu;
	GtkWidget *optmenu_menu;
	GtkWidget *menuitem;
	GtkWidget *recvatgetall_checkbtn;

	GtkWidget *hbox2;
	GtkWidget *frame2;
	GtkWidget *maxarticle_label;
	GtkWidget *maxarticle_spinbtn;
	GtkObject *maxarticle_spinbtn_adj;
	GtkTooltips *maxarticle_tool_tip;

	inbox_tooltip = gtk_tooltips_new();
	size_limit_tooltip = gtk_tooltips_new();

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	local_vbox = gtkut_get_options_frame(vbox1, &local_frame, _("Local"));

	local_hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (local_hbox);
	gtk_box_pack_start (GTK_BOX (local_vbox), local_hbox, FALSE, FALSE, 0);

	local_inbox_label = gtk_label_new (_("Default inbox"));
	gtk_widget_show (local_inbox_label);
	gtk_box_pack_start (GTK_BOX (local_hbox), local_inbox_label, FALSE, FALSE, 0);

	local_inbox_entry = gtk_entry_new ();
	gtk_widget_show (local_inbox_entry);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(inbox_tooltip), local_inbox_entry,
			     _("Unfiltered messages will be stored in this folder"),
			     NULL);
	gtk_widget_set_size_request (local_inbox_entry, DEFAULT_ENTRY_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (local_hbox), local_inbox_entry, TRUE, TRUE, 0);

	local_inbox_btn = gtkut_get_browse_file_btn(_("Bro_wse"));
	gtk_widget_show (local_inbox_btn);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(inbox_tooltip), local_inbox_btn,
			     _("Unfiltered messages will be stored in this folder"),
			     NULL);
	gtk_box_pack_start (GTK_BOX (local_hbox), local_inbox_btn, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (local_inbox_btn), "clicked",
			  G_CALLBACK (prefs_account_select_folder_cb),
			  local_inbox_entry);

	vbox2 = gtkut_get_options_frame(vbox1, &frame1, _("POP3"));
	PACK_CHECK_BUTTON (vbox2, use_apop_checkbtn,
			   _("Use secure authentication (APOP)"));

	PACK_CHECK_BUTTON (vbox2, rmmail_checkbtn,
			   _("Remove messages on server when received"));

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox1), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 12, -1);

	leave_time_label = gtk_label_new (_("Remove after"));
	gtk_widget_show (leave_time_label);
	gtk_box_pack_start (GTK_BOX (hbox1), leave_time_label, FALSE, FALSE, 0);

	leave_time_tooltip = gtk_tooltips_new();

	leave_time_entry = gtk_entry_new ();
	gtk_widget_show (leave_time_entry);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(leave_time_tooltip), leave_time_entry,
			     _("0 days: remove immediately"), NULL);
	gtk_widget_set_size_request (leave_time_entry, 64, -1);
	gtk_box_pack_start (GTK_BOX (hbox1), leave_time_entry, FALSE, FALSE, 0);

	leave_time_label = gtk_label_new (_("days"));
	gtk_widget_show (leave_time_label);
	gtk_box_pack_start (GTK_BOX (hbox1), leave_time_label, FALSE, FALSE, 0);

	SET_TOGGLE_SENSITIVITY (rmmail_checkbtn, hbox1);

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 0);

	PACK_CHECK_BUTTON (hbox1, size_limit_checkbtn, _("Receive size limit"));

	gtk_tooltips_set_tip(GTK_TOOLTIPS(size_limit_tooltip), size_limit_checkbtn,
			     _("Messages over this limit will be partially retrieved. "
		   	       "When selecting them you will be able to download them fully "
			       "or delete them."),
			     NULL);

	size_limit_entry = gtk_entry_new ();
	gtk_widget_show (size_limit_entry);
	gtk_widget_set_size_request (size_limit_entry, 64, -1);
	gtk_box_pack_start (GTK_BOX (hbox1), size_limit_entry, FALSE, FALSE, 0);

	label = gtk_label_new (_("KB"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

	SET_TOGGLE_SENSITIVITY (size_limit_checkbtn, size_limit_entry);

	PACK_VSPACER(vbox2, vbox3, VSPACING_NARROW_2);

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 0);

	inbox_label = gtk_label_new (_("Default inbox"));
	gtk_widget_show (inbox_label);
	gtk_box_pack_start (GTK_BOX (hbox1), inbox_label, FALSE, FALSE, 0);

	inbox_entry = gtk_entry_new ();
	gtk_widget_show (inbox_entry);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(inbox_tooltip), inbox_entry,
			     _("Unfiltered messages will be stored in this folder"),
			     NULL);
	gtk_widget_set_size_request (inbox_entry, DEFAULT_ENTRY_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (hbox1), inbox_entry, TRUE, TRUE, 0);

	inbox_btn = gtkut_get_browse_file_btn(_("Bro_wse"));
	gtk_widget_show (inbox_btn);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(inbox_tooltip), inbox_btn,
			     _("Unfiltered messages will be stored in this folder"),
			     NULL);
	gtk_box_pack_start (GTK_BOX (hbox1), inbox_btn, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (inbox_btn), "clicked",
			  G_CALLBACK (prefs_account_select_folder_cb),
			  inbox_entry);

	vbox2 = gtkut_get_options_frame(vbox1, &frame2, _("NNTP"));

	hbox2 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);

	maxarticle_label = gtk_label_new
		(_("Maximum number of articles to download"));
	gtk_widget_show (maxarticle_label);
	gtk_box_pack_start (GTK_BOX (hbox2), maxarticle_label, FALSE, FALSE, 0);

	maxarticle_tool_tip = gtk_tooltips_new();

	maxarticle_spinbtn_adj =
		gtk_adjustment_new (300, 0, 10000, 10, 100, 100);
	maxarticle_spinbtn = gtk_spin_button_new
		(GTK_ADJUSTMENT (maxarticle_spinbtn_adj), 10, 0);
	gtk_widget_show (maxarticle_spinbtn);
	gtk_tooltips_set_tip(maxarticle_tool_tip, maxarticle_spinbtn,
			     _("unlimited if 0 is specified"), NULL);
	gtk_box_pack_start (GTK_BOX (hbox2), maxarticle_spinbtn,
			    FALSE, FALSE, 0);
	gtk_widget_set_size_request (maxarticle_spinbtn, 64, -1);
	gtk_spin_button_set_numeric
		(GTK_SPIN_BUTTON (maxarticle_spinbtn), TRUE);

	vbox2 = gtkut_get_options_frame(vbox1, &imap_frame, _("IMAP4"));

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 0);

	label = gtk_label_new (_("Authentication method"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

	optmenu = gtk_option_menu_new ();
	gtk_widget_show (optmenu);
	gtk_box_pack_start (GTK_BOX (hbox1), optmenu, FALSE, FALSE, 0);

	optmenu_menu = gtk_menu_new ();

	MENUITEM_ADD (optmenu_menu, menuitem, _("Automatic"), 0);
	MENUITEM_ADD (optmenu_menu, menuitem, "LOGIN", IMAP_AUTH_LOGIN);
	MENUITEM_ADD (optmenu_menu, menuitem, "CRAM-MD5", IMAP_AUTH_CRAM_MD5);
	MENUITEM_ADD (optmenu_menu, menuitem, "ANONYMOUS", IMAP_AUTH_ANON);
	MENUITEM_ADD (optmenu_menu, menuitem, "GSSAPI", IMAP_AUTH_GSSAPI);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (optmenu), optmenu_menu);

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 4);

	imapdir_label = gtk_label_new (_("IMAP server directory"));
	gtk_widget_show (imapdir_label);
	gtk_box_pack_start (GTK_BOX (hbox1), imapdir_label, FALSE, FALSE, 0);

	imapdir_label = gtk_label_new(_("(usually empty)"));
	gtk_widget_show (imapdir_label);
	gtkut_widget_set_small_font_size (imapdir_label);
	gtk_box_pack_start (GTK_BOX (hbox1), imapdir_label, FALSE, FALSE, 0);

	imapdir_entry = gtk_entry_new();
	gtk_widget_show (imapdir_entry);
	gtk_box_pack_start (GTK_BOX (hbox1), imapdir_entry, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 4);

	PACK_CHECK_BUTTON (hbox1, subsonly_checkbtn,
			   _("Show subscribed folders only"));

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 4);

	PACK_CHECK_BUTTON (hbox1, low_bandwidth_checkbtn,
			   _("Bandwidth-efficient mode"));
	gtk_tooltips_set_tip(GTK_TOOLTIPS(inbox_tooltip), low_bandwidth_checkbtn,
			     _("This mode uses less bandwidth, but can be slower with some servers."),
			     NULL);

	PACK_CHECK_BUTTON (vbox1, filter_on_recv_checkbtn,
			   _("Filter messages on receiving"));

	PACK_CHECK_BUTTON
		(vbox1, recvatgetall_checkbtn,
		 _("'Get Mail' checks for new messages on this account"));

	page->pop3_frame               = frame1;
	page->use_apop_checkbtn          = use_apop_checkbtn;
	page->rmmail_checkbtn            = rmmail_checkbtn;
	page->leave_time_entry         = leave_time_entry;
	page->size_limit_checkbtn        = size_limit_checkbtn;
	page->size_limit_entry         = size_limit_entry;
	page->filter_on_recv_checkbtn    = filter_on_recv_checkbtn;
	page->inbox_label              = inbox_label;
	page->inbox_entry              = inbox_entry;
	page->inbox_btn                = inbox_btn;

	page->imap_frame               = imap_frame;
	page->imap_auth_type_optmenu   = optmenu;

	page->imapdir_label		= imapdir_label;
	page->imapdir_entry           = imapdir_entry;
	page->subsonly_checkbtn		= subsonly_checkbtn;
	page->low_bandwidth_checkbtn		= low_bandwidth_checkbtn;
	page->local_frame		= local_frame;
	page->local_inbox_label	= local_inbox_label;
	page->local_inbox_entry	= local_inbox_entry;
	page->local_inbox_btn		= local_inbox_btn;

	page->recvatgetall_checkbtn      = recvatgetall_checkbtn;

	page->frame_maxarticle	= frame2;
	page->maxarticle_spinbtn     	= maxarticle_spinbtn;
	page->maxarticle_spinbtn_adj 	= maxarticle_spinbtn_adj;

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(receive_param);
	} else
		prefs_set_dialog(receive_param);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}

static void send_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	SendPage *page = (SendPage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *frame;
	GtkWidget *msgid_checkbtn;
	GtkWidget *hbox;
	GtkWidget *customhdr_checkbtn;
	GtkWidget *customhdr_edit_btn;
	GtkWidget *vbox3;
	GtkWidget *smtp_auth_checkbtn;
	GtkWidget *optmenu;
	GtkWidget *optmenu_menu;
	GtkWidget *menuitem;
	GtkWidget *vbox4;
	GtkWidget *hbox_spc;
	GtkWidget *label;
	GtkWidget *smtp_uid_entry;
	GtkWidget *smtp_pass_entry;
	GtkWidget *vbox_spc;
	GtkWidget *pop_bfr_smtp_checkbtn;
	GtkWidget *pop_bfr_smtp_tm_entry;
	GtkWidget *pop_auth_timeout_lbl;
	GtkWidget *pop_auth_minutes_lbl;

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox2 = gtkut_get_options_frame(vbox1, &frame, _("Header"));

	PACK_CHECK_BUTTON (vbox2, msgid_checkbtn, _("Generate Message-ID"));

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	PACK_CHECK_BUTTON (hbox, customhdr_checkbtn,
			   _("Add user-defined header"));

	customhdr_edit_btn = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_widget_show (customhdr_edit_btn);
	gtk_box_pack_start (GTK_BOX (hbox), customhdr_edit_btn,
			    FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (customhdr_edit_btn), "clicked",
			  G_CALLBACK (prefs_account_edit_custom_header),
			  NULL);

	SET_TOGGLE_SENSITIVITY (customhdr_checkbtn, customhdr_edit_btn);

	vbox3 = gtkut_get_options_frame(vbox1, &frame, _("Authentication"));

	PACK_CHECK_BUTTON (vbox3, smtp_auth_checkbtn,
		_("SMTP Authentication (SMTP AUTH)"));

	vbox4 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox4);
	gtk_box_pack_start (GTK_BOX (vbox3), vbox4, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 12, -1);

	label = gtk_label_new (_("Authentication method"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	optmenu = gtk_option_menu_new ();
	gtk_widget_show (optmenu);
	gtk_box_pack_start (GTK_BOX (hbox), optmenu, FALSE, FALSE, 0);

	optmenu_menu = gtk_menu_new ();

	MENUITEM_ADD (optmenu_menu, menuitem, _("Automatic"), 0);
	MENUITEM_ADD (optmenu_menu, menuitem, "PLAIN", SMTPAUTH_PLAIN);
	MENUITEM_ADD (optmenu_menu, menuitem, "LOGIN", SMTPAUTH_LOGIN);
	MENUITEM_ADD (optmenu_menu, menuitem, "CRAM-MD5", SMTPAUTH_CRAM_MD5);
	MENUITEM_ADD (optmenu_menu, menuitem, "DIGEST-MD5", SMTPAUTH_DIGEST_MD5);
	gtk_widget_set_sensitive (menuitem, FALSE);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (optmenu), optmenu_menu);

	PACK_VSPACER(vbox4, vbox_spc, VSPACING_NARROW_2);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 12, -1);

	label = gtk_label_new (_("User ID"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	smtp_uid_entry = gtk_entry_new ();
	gtk_widget_show (smtp_uid_entry);
	gtk_widget_set_size_request (smtp_uid_entry, DEFAULT_ENTRY_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (hbox), smtp_uid_entry, TRUE, TRUE, 0);

#ifdef MAEMO
	PACK_VSPACER(vbox4, vbox_spc, VSPACING_NARROW_2);
	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 12, -1);
#endif
	label = gtk_label_new (_("Password"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	smtp_pass_entry = gtk_entry_new ();
	gtk_widget_show (smtp_pass_entry);
	gtk_widget_set_size_request (smtp_pass_entry, DEFAULT_ENTRY_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (hbox), smtp_pass_entry, TRUE, TRUE, 0);

	gtk_entry_set_visibility (GTK_ENTRY (smtp_pass_entry), FALSE);
	PACK_VSPACER(vbox4, vbox_spc, VSPACING_NARROW_2);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox4), hbox, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 12, -1);

	label = gtk_label_new
		(_("If you leave these entries empty, the same "
		   "user ID and password as receiving will be used."));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtkut_widget_set_small_font_size (label);

	SET_TOGGLE_SENSITIVITY (smtp_auth_checkbtn, vbox4);

	PACK_CHECK_BUTTON (vbox3, pop_bfr_smtp_checkbtn,
		_("Authenticate with POP3 before sending"));
	
	g_signal_connect (G_OBJECT (pop_bfr_smtp_checkbtn), "clicked",
			  G_CALLBACK (pop_bfr_smtp_tm_set_sens),
			  NULL);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 12, -1);

	pop_auth_timeout_lbl = gtk_label_new(_("POP authentication timeout: "));
	gtk_widget_show (pop_auth_timeout_lbl);
	gtk_box_pack_start (GTK_BOX (hbox), pop_auth_timeout_lbl, FALSE, FALSE, 0);

	pop_bfr_smtp_tm_entry = gtk_entry_new ();
	gtk_widget_show (pop_bfr_smtp_tm_entry);
	gtk_widget_set_size_request (pop_bfr_smtp_tm_entry, 30, -1);
	gtk_box_pack_start (GTK_BOX (hbox), pop_bfr_smtp_tm_entry, FALSE, FALSE, 0);

	pop_auth_minutes_lbl = gtk_label_new(_("minutes"));
	gtk_widget_show (pop_auth_minutes_lbl);
	gtk_box_pack_start (GTK_BOX (hbox), pop_auth_minutes_lbl, FALSE, FALSE, 0);
	
	page->msgid_checkbtn     = msgid_checkbtn;
	page->customhdr_checkbtn = customhdr_checkbtn;

	page->smtp_auth_checkbtn       = smtp_auth_checkbtn;
	page->smtp_auth_type_optmenu = optmenu;
	page->smtp_uid_entry         = smtp_uid_entry;
	page->smtp_pass_entry        = smtp_pass_entry;
	page->pop_bfr_smtp_checkbtn    = pop_bfr_smtp_checkbtn;
	page->pop_bfr_smtp_tm_entry  = pop_bfr_smtp_tm_entry;
	page->pop_auth_timeout_lbl   = pop_auth_timeout_lbl;
	page->pop_auth_minutes_lbl   = pop_auth_minutes_lbl;

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(send_param);
	} else
		prefs_set_dialog(send_param);

	pop_bfr_smtp_tm_set_sens (NULL, NULL);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}
	
static void compose_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	ComposePage *page = (ComposePage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;
	GtkWidget *sig_hbox;
	GtkWidget *hbox1;
	GtkWidget *hbox2;
	GtkWidget *frame_sig;
	GtkWidget *vbox_sig;
	GtkWidget *label_sigpath;
	GtkWidget *checkbtn_autosig;
	GtkWidget *label_sigsep;
	GtkWidget *entry_sigsep;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *autocc_checkbtn;
	GtkWidget *autocc_entry;
	GtkWidget *autobcc_checkbtn;
	GtkWidget *autobcc_entry;
	GtkWidget *autoreplyto_checkbtn;
	GtkWidget *autoreplyto_entry;
#if USE_ASPELL
	GtkWidget *frame_dict;
	GtkWidget *table_dict;
	GtkWidget *checkbtn_enable_default_dictionary = NULL;
	GtkWidget *optmenu_default_dictionary = NULL;
	GtkWidget *checkbtn_enable_default_alt_dictionary = NULL;
	GtkWidget *optmenu_default_alt_dictionary = NULL;
#endif

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox_sig = gtkut_get_options_frame(vbox1, &frame_sig, _("Signature"));

	PACK_CHECK_BUTTON (vbox_sig, checkbtn_autosig,
			   _("Insert signature automatically"));

	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox_sig), hbox1, TRUE, TRUE, 0);
	label_sigsep = gtk_label_new (_("Signature separator"));
	gtk_widget_show (label_sigsep);
	gtk_box_pack_start (GTK_BOX (hbox1), label_sigsep, FALSE, FALSE, 0);

	entry_sigsep = gtk_entry_new ();
	gtk_widget_show (entry_sigsep);
	gtk_box_pack_start (GTK_BOX (hbox1), entry_sigsep, FALSE, FALSE, 0);

	gtk_widget_set_size_request (entry_sigsep, 64, -1);

	sig_hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (sig_hbox);
	gtk_box_pack_start (GTK_BOX (vbox_sig), sig_hbox, FALSE, FALSE, 0);

	sigfile_radiobtn = gtk_radio_button_new_with_label (NULL, _("File"));
	gtk_widget_show (sigfile_radiobtn);
	gtk_box_pack_start (GTK_BOX (sig_hbox), sigfile_radiobtn,
			    FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (sigfile_radiobtn),
			   MENU_VAL_ID,
			   GINT_TO_POINTER (SIG_FILE));
	g_signal_connect(G_OBJECT(sigfile_radiobtn), "clicked",
			 G_CALLBACK(prefs_account_sigfile_radiobtn_cb), NULL);

	sigcmd_radiobtn = gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON(sigfile_radiobtn), _("Command output"));
	gtk_widget_show (sigcmd_radiobtn);
	gtk_box_pack_start (GTK_BOX (sig_hbox), sigcmd_radiobtn,
			    FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (sigcmd_radiobtn),
			   MENU_VAL_ID,
			   GINT_TO_POINTER (SIG_COMMAND));
	g_signal_connect(G_OBJECT(sigcmd_radiobtn), "clicked",
			 G_CALLBACK(prefs_account_sigcmd_radiobtn_cb), NULL);

	hbox2 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox_sig), hbox2, TRUE, TRUE, 0);
	label_sigpath = gtk_label_new (_("Signature"));
	gtk_widget_show (label_sigpath);
	gtk_box_pack_start (GTK_BOX (hbox2), label_sigpath, FALSE, FALSE, 0);

	entry_sigpath = gtk_entry_new ();
	gtk_widget_show (entry_sigpath);
	gtk_box_pack_start (GTK_BOX (hbox2), entry_sigpath, TRUE, TRUE, 0);

	signature_browse_button = gtkut_get_browse_file_btn(_("Bro_wse"));
	gtk_widget_show (signature_browse_button);
	gtk_box_pack_start (GTK_BOX (hbox2), signature_browse_button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(signature_browse_button), "clicked",
			 G_CALLBACK(prefs_account_signature_browse_cb), NULL);

	signature_edit_button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_widget_show (signature_edit_button);
	gtk_box_pack_start (GTK_BOX (hbox2), signature_edit_button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(signature_edit_button), "clicked",
			 G_CALLBACK(prefs_account_signature_edit_cb), entry_sigpath);

	PACK_FRAME (vbox1, frame, _("Automatically set the following addresses"));

	table =  gtk_table_new (3, 2, FALSE);
	gtk_widget_show (table);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_container_set_border_width (GTK_CONTAINER (table), 8);
	gtk_table_set_row_spacings (GTK_TABLE (table), VSPACING_NARROW_2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 8);

	autocc_checkbtn = gtk_check_button_new_with_label (_("Cc"));
	gtk_widget_show (autocc_checkbtn);
	gtk_table_attach (GTK_TABLE (table), autocc_checkbtn, 0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);

	autocc_entry = gtk_entry_new ();
	gtk_widget_show (autocc_entry);
	gtk_table_attach (GTK_TABLE (table), autocc_entry, 1, 2, 0, 1,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	SET_TOGGLE_SENSITIVITY (autocc_checkbtn, autocc_entry);

	autobcc_checkbtn = gtk_check_button_new_with_label (_("Bcc"));
	gtk_widget_show (autobcc_checkbtn);
	gtk_table_attach (GTK_TABLE (table), autobcc_checkbtn, 0, 1, 1, 2,
			  GTK_FILL, 0, 0, 0);

	autobcc_entry = gtk_entry_new ();
	gtk_widget_show (autobcc_entry);
	gtk_table_attach (GTK_TABLE (table), autobcc_entry, 1, 2, 1, 2,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	SET_TOGGLE_SENSITIVITY (autobcc_checkbtn, autobcc_entry);

	autoreplyto_checkbtn = gtk_check_button_new_with_label (_("Reply-To"));
	gtk_widget_show (autoreplyto_checkbtn);
	gtk_table_attach (GTK_TABLE (table), autoreplyto_checkbtn, 0, 1, 2, 3,
			  GTK_FILL, 0, 0, 0);

	autoreplyto_entry = gtk_entry_new ();
	gtk_widget_show (autoreplyto_entry);
	gtk_table_attach (GTK_TABLE (table), autoreplyto_entry, 1, 2, 2, 3,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	SET_TOGGLE_SENSITIVITY (autoreplyto_checkbtn, autoreplyto_entry);

#if USE_ASPELL
	PACK_FRAME (vbox1, frame_dict, _("Spell check dictionaries"));

	table_dict =  gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table_dict);
	gtk_container_add (GTK_CONTAINER (frame_dict), table_dict);
	gtk_container_set_border_width (GTK_CONTAINER (table_dict), 8);
	gtk_table_set_row_spacings (GTK_TABLE (table_dict), VSPACING_NARROW_2);
	gtk_table_set_col_spacings (GTK_TABLE (table_dict), 8);

	/* Default dictionary */
	checkbtn_enable_default_dictionary = gtk_check_button_new_with_label(_("Default dictionary"));
	gtk_table_attach(GTK_TABLE(table_dict), checkbtn_enable_default_dictionary, 0, 1,
			0, 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_enable_default_dictionary),
			tmp_ac_prefs.enable_default_dictionary);

	optmenu_default_dictionary = gtk_option_menu_new();
	gtk_table_attach(GTK_TABLE(table_dict), optmenu_default_dictionary, 1, 2,
			0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optmenu_default_dictionary), 
			gtkaspell_dictionary_option_menu_new(
			prefs_common.aspell_path));

	SET_TOGGLE_SENSITIVITY(checkbtn_enable_default_dictionary, optmenu_default_dictionary);

	/* Default dictionary */
	checkbtn_enable_default_alt_dictionary = gtk_check_button_new_with_label(_("Default alternate dictionary"));
	gtk_table_attach(GTK_TABLE(table_dict), checkbtn_enable_default_alt_dictionary, 0, 1,
			1, 2, GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_enable_default_alt_dictionary),
			tmp_ac_prefs.enable_default_alt_dictionary);

	optmenu_default_alt_dictionary = gtk_option_menu_new();
	gtk_table_attach(GTK_TABLE(table_dict), optmenu_default_alt_dictionary, 1, 2,
			1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(optmenu_default_alt_dictionary), 
			gtkaspell_dictionary_option_menu_new_with_refresh(
			prefs_common.aspell_path, FALSE));

	SET_TOGGLE_SENSITIVITY(checkbtn_enable_default_alt_dictionary, optmenu_default_alt_dictionary);

	gtk_widget_show_all(table_dict);
#endif

	page->sigfile_radiobtn = sigfile_radiobtn;
	page->entry_sigpath      = entry_sigpath;
	page->checkbtn_autosig   = checkbtn_autosig;
	page->entry_sigsep       = entry_sigsep;

	page->autocc_checkbtn      = autocc_checkbtn;
	page->autocc_entry       = autocc_entry;
	page->autobcc_checkbtn     = autobcc_checkbtn;
	page->autobcc_entry      = autobcc_entry;
	page->autoreplyto_checkbtn = autoreplyto_checkbtn;
	page->autoreplyto_entry  = autoreplyto_entry;
#ifdef USE_ASPELL
	page->checkbtn_enable_default_dictionary = checkbtn_enable_default_dictionary;
	page->optmenu_default_dictionary = optmenu_default_dictionary;
	page->checkbtn_enable_default_alt_dictionary = checkbtn_enable_default_alt_dictionary;
	page->optmenu_default_alt_dictionary = optmenu_default_alt_dictionary;
#endif

#ifdef USE_ASPELL
	/* reset gtkaspell menus */
	if (compose_page.optmenu_default_dictionary != NULL) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(compose_page.optmenu_default_dictionary));
		gtk_option_menu_set_menu(GTK_OPTION_MENU(compose_page.optmenu_default_dictionary), 
				gtkaspell_dictionary_option_menu_new(
				prefs_common.aspell_path));
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(compose_page.optmenu_default_alt_dictionary));
		gtk_option_menu_set_menu(GTK_OPTION_MENU(compose_page.optmenu_default_alt_dictionary), 
				gtkaspell_dictionary_option_menu_new_with_refresh(
				prefs_common.aspell_path, FALSE));
	}
#endif

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(compose_param);
	} else
		prefs_set_dialog(compose_param);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}
	
static void templates_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	TemplatesPage *page = (TemplatesPage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;
	GtkWidget *vbox;

	vbox = gtk_vbox_new(FALSE, VSPACING);
	gtk_widget_show(vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), VBOX_BORDER);

	/* compose/reply/forward formats */
	quotefmt_create_new_msg_fmt_widgets(
				window,
				vbox,
				&page->checkbtn_compose_with_format,
				_("Use a specific format for new messages"),
				&page->compose_subject_format,
				&page->compose_body_format,
				FALSE);
	quotefmt_create_reply_fmt_widgets(
				window,
				vbox,
				&page->checkbtn_reply_with_format,
				_("Use a specific reply quote format"),
				&page->reply_quotemark,
				&page->reply_body_format,
				FALSE);
	quotefmt_create_forward_fmt_widgets(
				window,
				vbox,
				&page->checkbtn_forward_with_format,
				_("Use a specific forward quote format"),
				&page->forward_quotemark,
				&page->forward_body_format,
				FALSE);
	quotefmt_add_info_button(window, vbox);

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(templates_param);
	} else
		prefs_set_dialog(templates_param);

	page->vbox = vbox;

	page->page.widget = vbox;
}

static void privacy_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	PrivacyPage *page = (PrivacyPage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *hbox1;
	GtkWidget *label;
	GtkWidget *default_privacy_system;
	GtkWidget *default_encrypt_checkbtn;
	GtkWidget *default_encrypt_reply_checkbtn;
	GtkWidget *default_sign_checkbtn;
	GtkWidget *save_clear_text_checkbtn;
	GtkWidget *encrypt_to_self_checkbtn;

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

	hbox1 = gtk_hbox_new(FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_container_add (GTK_CONTAINER(vbox2), hbox1);

	label = gtk_label_new(_("Default privacy system"));
	gtk_widget_show(label);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);

	default_privacy_system = gtk_option_menu_new();
	gtk_widget_show (default_privacy_system);
	gtk_box_pack_start (GTK_BOX(hbox1), default_privacy_system, FALSE, TRUE, 0);

	PACK_CHECK_BUTTON (vbox2, default_sign_checkbtn,
			   _("Always sign messages"));
	PACK_CHECK_BUTTON (vbox2, default_encrypt_checkbtn,
			   _("Always encrypt messages"));
	PACK_CHECK_BUTTON (vbox2, default_encrypt_reply_checkbtn,
			   _("Always encrypt messages when replying to an "
			     "encrypted message"));
	PACK_CHECK_BUTTON (vbox2, encrypt_to_self_checkbtn,
			   _("Encrypt sent messages with your own key in addition to recipient's"));
	PACK_CHECK_BUTTON (vbox2, save_clear_text_checkbtn,
			   _("Save sent encrypted messages as clear text"));

	SET_TOGGLE_SENSITIVITY_REVERSE(encrypt_to_self_checkbtn, save_clear_text_checkbtn);
	SET_TOGGLE_SENSITIVITY_REVERSE(save_clear_text_checkbtn, encrypt_to_self_checkbtn);

	page->default_privacy_system = default_privacy_system;
	page->default_encrypt_checkbtn = default_encrypt_checkbtn;
	page->default_encrypt_reply_checkbtn = default_encrypt_reply_checkbtn;
	page->default_sign_checkbtn    = default_sign_checkbtn;
	page->save_clear_text_checkbtn = save_clear_text_checkbtn;
	page->encrypt_to_self_checkbtn = encrypt_to_self_checkbtn;

	update_privacy_system_menu();

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(privacy_param);
	} else
		prefs_set_dialog(privacy_param);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}
	
#if USE_OPENSSL

#define CREATE_RADIO_BUTTON(box, btn, btn_p, label, data)		\
{									\
	btn = gtk_radio_button_new_with_label_from_widget		\
		(GTK_RADIO_BUTTON (btn_p), label);			\
	gtk_widget_show (btn);						\
	gtk_box_pack_start (GTK_BOX (box), btn, FALSE, FALSE, 0);	\
	g_object_set_data (G_OBJECT (btn),				\
			   MENU_VAL_ID,					\
			   GINT_TO_POINTER (data));			\
}

#define CREATE_RADIO_BUTTONS(box,					\
			     btn1, btn1_label, btn1_data,		\
			     btn2, btn2_label, btn2_data,		\
			     btn3, btn3_label, btn3_data)		\
{									\
	btn1 = gtk_radio_button_new_with_label(NULL, btn1_label);	\
	gtk_widget_show (btn1);						\
	gtk_box_pack_start (GTK_BOX (box), btn1, FALSE, FALSE, 0);	\
	g_object_set_data (G_OBJECT (btn1),				\
			   MENU_VAL_ID,					\
			   GINT_TO_POINTER (btn1_data));		\
									\
	CREATE_RADIO_BUTTON(box, btn2, btn1, btn2_label, btn2_data);	\
	CREATE_RADIO_BUTTON(box, btn3, btn1, btn3_label, btn3_data);	\
}

static void ssl_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	SSLPage *page = (SSLPage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;

	GtkWidget *pop_frame;
	GtkWidget *vbox2;
	GtkWidget *pop_nossl_radiobtn;
	GtkWidget *pop_ssltunnel_radiobtn;
	GtkWidget *pop_starttls_radiobtn;

	GtkWidget *imap_frame;
	GtkWidget *vbox3;
	GtkWidget *imap_nossl_radiobtn;
	GtkWidget *imap_ssltunnel_radiobtn;
	GtkWidget *imap_starttls_radiobtn;

	GtkWidget *nntp_frame;
	GtkWidget *vbox4;
	GtkWidget *nntp_nossl_radiobtn;
	GtkWidget *nntp_ssltunnel_radiobtn;

	GtkWidget *send_frame;
	GtkWidget *vbox5;
	GtkWidget *smtp_nossl_radiobtn;
	GtkWidget *smtp_ssltunnel_radiobtn;
	GtkWidget *smtp_starttls_radiobtn;

	GtkWidget *vbox6;
	GtkWidget *use_nonblocking_ssl_checkbtn;
	GtkWidget *hbox;
	GtkWidget *hbox_spc;
	GtkWidget *label;

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox2 = gtkut_get_options_frame(vbox1, &pop_frame, _("POP3"));

	CREATE_RADIO_BUTTONS(vbox2,
			     pop_nossl_radiobtn,
			     _("Don't use SSL"),
			     SSL_NONE,
			     pop_ssltunnel_radiobtn,
			     _("Use SSL for POP3 connection"),
			     SSL_TUNNEL,
			     pop_starttls_radiobtn,
			     _("Use STARTTLS command to start SSL session"),
			     SSL_STARTTLS);

	vbox3 = gtkut_get_options_frame(vbox1, &imap_frame, _("IMAP4"));

	CREATE_RADIO_BUTTONS(vbox3,
			     imap_nossl_radiobtn,
			     _("Don't use SSL"),
			     SSL_NONE,
			     imap_ssltunnel_radiobtn,
			     _("Use SSL for IMAP4 connection"),
			     SSL_TUNNEL,
			     imap_starttls_radiobtn,
			     _("Use STARTTLS command to start SSL session"),
			     SSL_STARTTLS);

	vbox4 = gtkut_get_options_frame(vbox1, &nntp_frame, _("NNTP"));

	nntp_nossl_radiobtn =
		gtk_radio_button_new_with_label (NULL, _("Don't use SSL"));
	gtk_widget_show (nntp_nossl_radiobtn);
	gtk_box_pack_start (GTK_BOX (vbox4), nntp_nossl_radiobtn,
			    FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (nntp_nossl_radiobtn),
			   MENU_VAL_ID,
			   GINT_TO_POINTER (SSL_NONE));

	CREATE_RADIO_BUTTON(vbox4, nntp_ssltunnel_radiobtn, nntp_nossl_radiobtn,
			    _("Use SSL for NNTP connection"), SSL_TUNNEL);

	vbox5 = gtkut_get_options_frame(vbox1, &send_frame, _("Send (SMTP)"));

	CREATE_RADIO_BUTTONS(vbox5,
			     smtp_nossl_radiobtn,
			     _("Don't use SSL (but, if necessary, use STARTTLS)"),
			     SSL_NONE,
			     smtp_ssltunnel_radiobtn,
			     _("Use SSL for SMTP connection"),
			     SSL_TUNNEL,
			     smtp_starttls_radiobtn,
			     _("Use STARTTLS command to start SSL session"),
			     SSL_STARTTLS);

	vbox6 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox6);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox6, FALSE, FALSE, 0);

	PACK_CHECK_BUTTON(vbox6, use_nonblocking_ssl_checkbtn,
			  _("Use non-blocking SSL"));

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox6), hbox, FALSE, FALSE, 0);

	hbox_spc = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_spc);
	gtk_box_pack_start (GTK_BOX (hbox), hbox_spc, FALSE, FALSE, 0);
	gtk_widget_set_size_request (hbox_spc, 16, -1);

	label = gtk_label_new
		(_("Turn this off if you have SSL connection problems"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtkut_widget_set_small_font_size (label);

	page->pop_frame               = pop_frame;
	page->pop_nossl_radiobtn      = pop_nossl_radiobtn;
	page->pop_ssltunnel_radiobtn  = pop_ssltunnel_radiobtn;
	page->pop_starttls_radiobtn   = pop_starttls_radiobtn;

	page->imap_frame              = imap_frame;
	page->imap_nossl_radiobtn     = imap_nossl_radiobtn;
	page->imap_ssltunnel_radiobtn = imap_ssltunnel_radiobtn;
	page->imap_starttls_radiobtn  = imap_starttls_radiobtn;

	page->nntp_frame              = nntp_frame;
	page->nntp_nossl_radiobtn     = nntp_nossl_radiobtn;
	page->nntp_ssltunnel_radiobtn = nntp_ssltunnel_radiobtn;

	page->send_frame              = send_frame;
	page->smtp_nossl_radiobtn     = smtp_nossl_radiobtn;
	page->smtp_ssltunnel_radiobtn = smtp_ssltunnel_radiobtn;
	page->smtp_starttls_radiobtn  = smtp_starttls_radiobtn;

	page->use_nonblocking_ssl_checkbtn = use_nonblocking_ssl_checkbtn;

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(ssl_param);
	} else
		prefs_set_dialog(ssl_param);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}

#undef CREATE_RADIO_BUTTONS
#undef CREATE_RADIO_BUTTON
#endif /* USE_OPENSSL */
	
static void advanced_create_widget_func(PrefsPage * _page,
                                           GtkWindow * window,
                                           gpointer data)
{
	AdvancedPage *page = (AdvancedPage *) _page;
	PrefsAccount *ac_prefs = (PrefsAccount *) data;

	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *hbox1;
	GtkWidget *checkbtn_smtpport;
	GtkWidget *entry_smtpport;
	GtkWidget *hbox_popport;
	GtkWidget *checkbtn_popport;
	GtkWidget *entry_popport;
	GtkWidget *hbox_imapport;
	GtkWidget *checkbtn_imapport;
	GtkWidget *entry_imapport;
	GtkWidget *hbox_nntpport;
	GtkWidget *checkbtn_nntpport;
	GtkWidget *entry_nntpport;
	GtkWidget *checkbtn_domain;
	GtkWidget *entry_domain;
	GtkWidget *checkbtn_crosspost;
 	GtkWidget *colormenu_crosspost;
 	GtkWidget *menu;
	GtkWidget *checkbtn_tunnelcmd;
	GtkWidget *entry_tunnelcmd;
	GtkWidget *folder_frame;
	GtkWidget *vbox3;
	GtkWidget *table;
	GtkWidget *sent_folder_checkbtn;
	GtkWidget *sent_folder_entry;
	GtkWidget *queue_folder_checkbtn;
	GtkWidget *queue_folder_entry;
	GtkWidget *draft_folder_checkbtn;
	GtkWidget *draft_folder_entry;
	GtkWidget *trash_folder_checkbtn;
	GtkWidget *trash_folder_entry;

#define PACK_HBOX(hbox) \
	{ \
	hbox = gtk_hbox_new (FALSE, 8); \
	gtk_widget_show (hbox); \
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0); \
	}

#define PACK_PORT_ENTRY(box, entry) \
	{ \
	entry = gtk_entry_new (); \
	gtk_entry_set_max_length  (GTK_ENTRY(entry), 5); \
	gtk_widget_show (entry); \
	gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0); \
	gtk_widget_set_size_request (entry, 64, -1); \
	}

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox2 = gtk_vbox_new (FALSE, VSPACING_NARROW_2);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

	PACK_HBOX (hbox1);
	PACK_CHECK_BUTTON (hbox1, checkbtn_smtpport, _("SMTP port"));
	PACK_PORT_ENTRY (hbox1, entry_smtpport);
	SET_TOGGLE_SENSITIVITY (checkbtn_smtpport, entry_smtpport);

	PACK_HBOX (hbox_popport);
	PACK_CHECK_BUTTON (hbox_popport, checkbtn_popport,
			   _("POP3 port"));
	PACK_PORT_ENTRY (hbox_popport, entry_popport);
	SET_TOGGLE_SENSITIVITY (checkbtn_popport, entry_popport);

	PACK_HBOX (hbox_imapport);
	PACK_CHECK_BUTTON (hbox_imapport, checkbtn_imapport,
			   _("IMAP4 port"));
	PACK_PORT_ENTRY (hbox_imapport, entry_imapport);
	SET_TOGGLE_SENSITIVITY (checkbtn_imapport, entry_imapport);

	PACK_HBOX (hbox_nntpport);
	PACK_CHECK_BUTTON (hbox_nntpport, checkbtn_nntpport,
			   _("NNTP port"));
	PACK_PORT_ENTRY (hbox_nntpport, entry_nntpport);
	SET_TOGGLE_SENSITIVITY (checkbtn_nntpport, entry_nntpport);

	PACK_HBOX (hbox1);
	PACK_CHECK_BUTTON (hbox1, checkbtn_domain, _("Domain name"));

	entry_domain = gtk_entry_new ();
	gtk_widget_show (entry_domain);
	gtk_box_pack_start (GTK_BOX (hbox1), entry_domain, TRUE, TRUE, 0);
	SET_TOGGLE_SENSITIVITY (checkbtn_domain, entry_domain);

	
	PACK_HBOX (hbox1);
	PACK_CHECK_BUTTON (hbox1, checkbtn_tunnelcmd,
			   _("Use command to communicate with server"));
	entry_tunnelcmd = gtk_entry_new ();
	gtk_widget_show (entry_tunnelcmd);
	gtk_box_pack_start (GTK_BOX (hbox1), entry_tunnelcmd, TRUE, TRUE, 0);
	SET_TOGGLE_SENSITIVITY (checkbtn_tunnelcmd, entry_tunnelcmd);

	PACK_HBOX (hbox1);
	PACK_CHECK_BUTTON (hbox1, checkbtn_crosspost, 
			   _("Mark cross-posted messages as read and color:"));
	g_signal_connect (G_OBJECT (checkbtn_crosspost), "toggled",
			  G_CALLBACK (crosspost_color_toggled),
			  NULL);

	colormenu_crosspost = gtk_option_menu_new();
	gtk_widget_show (colormenu_crosspost);
	gtk_box_pack_start (GTK_BOX (hbox1), colormenu_crosspost, FALSE, FALSE, 0);

	menu = colorlabel_create_color_menu();
	gtk_option_menu_set_menu (GTK_OPTION_MENU(colormenu_crosspost), menu);
	SET_TOGGLE_SENSITIVITY(checkbtn_crosspost, colormenu_crosspost);

	PACK_HBOX (hbox1);
#undef PACK_HBOX
#undef PACK_PORT_ENTRY

	/* special folder setting (maybe these options are redundant) */

	vbox3 = gtkut_get_options_frame(vbox1, &folder_frame, _("Folder"));

	table = gtk_table_new (4, 3, FALSE);
	gtk_widget_show (table);
	gtk_container_add (GTK_CONTAINER (vbox3), table);
	gtk_table_set_row_spacings (GTK_TABLE (table), VSPACING_NARROW_2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);

#define SET_CHECK_BTN_AND_ENTRY(label, checkbtn, entry, n)		\
{									\
	GtkWidget *button;						\
									\
	checkbtn = gtk_check_button_new_with_label (label);		\
	gtk_widget_show (checkbtn);					\
	gtk_table_attach (GTK_TABLE (table), checkbtn,			\
			  0, 1, n, n + 1, GTK_FILL, 0, 0, 0);		\
									\
	entry = gtk_entry_new ();					\
	gtk_widget_show (entry);					\
	gtk_table_attach (GTK_TABLE (table), entry, 1, 2, n, n + 1,	\
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL,		\
			  GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);	\
									\
	button = gtkut_get_browse_file_btn(_("Browse"));		\
	gtk_widget_show (button);					\
	gtk_table_attach (GTK_TABLE (table), button,			\
			  2, 3, n, n + 1, GTK_FILL, 0, 0, 0);		\
	g_signal_connect						\
		(G_OBJECT (button), "clicked",			\
		 G_CALLBACK (prefs_account_select_folder_cb),		\
		 entry);						\
									\
	SET_TOGGLE_SENSITIVITY (checkbtn, entry);				\
	SET_TOGGLE_SENSITIVITY (checkbtn, button);			\
}

	SET_CHECK_BTN_AND_ENTRY(_("Put sent messages in"),
				sent_folder_checkbtn, sent_folder_entry, 0);
	SET_CHECK_BTN_AND_ENTRY(_("Put queued messages in"),
				queue_folder_checkbtn, queue_folder_entry, 1);
	SET_CHECK_BTN_AND_ENTRY(_("Put draft messages in"),
				draft_folder_checkbtn, draft_folder_entry, 2);
	SET_CHECK_BTN_AND_ENTRY(_("Put deleted messages in"),
				trash_folder_checkbtn, trash_folder_entry, 3);

	page->smtpport_checkbtn	= checkbtn_smtpport;
	page->smtpport_entry		= entry_smtpport;
	page->popport_hbox		= hbox_popport;
	page->popport_checkbtn		= checkbtn_popport;
	page->popport_entry		= entry_popport;
	page->imapport_hbox		= hbox_imapport;
	page->imapport_checkbtn	= checkbtn_imapport;
	page->imapport_entry		= entry_imapport;
	page->nntpport_hbox		= hbox_nntpport;
	page->nntpport_checkbtn	= checkbtn_nntpport;
	page->nntpport_entry		= entry_nntpport;
	page->domain_checkbtn		= checkbtn_domain;
	page->domain_entry		= entry_domain;
 	page->crosspost_checkbtn	= checkbtn_crosspost;
 	page->crosspost_colormenu	= colormenu_crosspost;

	page->tunnelcmd_checkbtn	= checkbtn_tunnelcmd;
	page->tunnelcmd_entry	= entry_tunnelcmd;

	page->sent_folder_checkbtn  = sent_folder_checkbtn;
	page->sent_folder_entry   = sent_folder_entry;
	page->queue_folder_checkbtn  = queue_folder_checkbtn;
	page->queue_folder_entry   = queue_folder_entry;
	page->draft_folder_checkbtn = draft_folder_checkbtn;
	page->draft_folder_entry  = draft_folder_entry;
	page->trash_folder_checkbtn = trash_folder_checkbtn;
	page->trash_folder_entry  = trash_folder_entry;

	tmp_ac_prefs = *ac_prefs;

	if (new_account) {
		prefs_set_dialog_to_default(advanced_param);
	} else
		prefs_set_dialog(advanced_param);

	page->vbox = vbox1;

	page->page.widget = vbox1;
}
	
static gint prefs_basic_apply(void)
{
	RecvProtocol protocol;
	gchar *old_id = NULL;
	gchar *new_id = NULL;
	struct BasicProtocol *protocol_optmenu = (struct BasicProtocol *) basic_page.protocol_optmenu;
	GtkWidget *optmenu = protocol_optmenu->combobox;

	protocol = combobox_get_active_data(GTK_COMBO_BOX(optmenu));

	if (*gtk_entry_get_text(GTK_ENTRY(basic_page.acname_entry)) == '\0') {
		alertpanel_error(_("Account name is not entered."));
		return -1;
	}
	if (*gtk_entry_get_text(GTK_ENTRY(basic_page.addr_entry)) == '\0') {
		alertpanel_error(_("Mail address is not entered."));
		return -1;
	}
	if (((protocol == A_POP3) || 
	     (protocol == A_LOCAL && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(basic_page.mailcmd_checkbtn))) || 
	     (protocol == A_NONE)) &&
           *gtk_entry_get_text(GTK_ENTRY(basic_page.smtpserv_entry)) == '\0') {
		alertpanel_error(_("SMTP server is not entered."));
		return -1;
	}
	if ((protocol == A_POP3 || protocol == A_IMAP4) &&
	    *gtk_entry_get_text(GTK_ENTRY(basic_page.uid_entry)) == '\0') {
		alertpanel_error(_("User ID is not entered."));
		return -1;
	}
	if (protocol == A_POP3 &&
	    *gtk_entry_get_text(GTK_ENTRY(basic_page.recvserv_entry)) == '\0') {
		alertpanel_error(_("POP3 server is not entered."));
		return -1;
	}
	if (protocol == A_POP3 || protocol == A_LOCAL) {
		GtkWidget *inbox_entry = (protocol == A_POP3 ? receive_page.inbox_entry : receive_page.local_inbox_entry );
		const gchar *mailbox = gtk_entry_get_text(GTK_ENTRY(inbox_entry));
		FolderItem *inbox =  folder_find_item_from_identifier(mailbox);
		if (!inbox && create_mailbox) {
			gchar *id = NULL;
			setup_write_mailbox_path(mainwindow_get_mainwindow(), "Mail");
			id = folder_item_get_identifier(folder_get_default_inbox_for_class(F_MH));
			gtk_entry_set_text(GTK_ENTRY(receive_page.inbox_entry),
				id);
			gtk_entry_set_text(GTK_ENTRY(receive_page.local_inbox_entry),
				id);
			g_free(id);
			mailbox = gtk_entry_get_text(GTK_ENTRY(inbox_entry));
			create_mailbox = FALSE;
			inbox =  folder_find_item_from_identifier(mailbox);
		}
	    	if (inbox == NULL) {
			alertpanel_error(_("The default inbox folder doesn't exist."));
			return -1;
		}
	}
	if (protocol == A_IMAP4 &&
	    *gtk_entry_get_text(GTK_ENTRY(basic_page.recvserv_entry)) == '\0') {
		alertpanel_error(_("IMAP4 server is not entered."));
		return -1;
	}
	if (protocol == A_NNTP &&
	    *gtk_entry_get_text(GTK_ENTRY(basic_page.nntpserv_entry)) == '\0') {
		alertpanel_error(_("NNTP server is not entered."));
		return -1;
	}

	if (protocol == A_LOCAL &&
	    *gtk_entry_get_text(GTK_ENTRY(basic_page.localmbox_entry)) == '\0') {
		alertpanel_error(_("local mailbox filename is not entered."));
		return -1;
	}

	if (protocol == A_LOCAL &&
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(basic_page.mailcmd_checkbtn)) && *gtk_entry_get_text(GTK_ENTRY(basic_page.mailcmd_entry)) == '\0') {
		alertpanel_error(_("mail command is not entered."));
		return -1;
	}
	
	if (protocol == A_IMAP4 || protocol == A_NNTP) 
		old_id = g_strdup_printf("#%s/%s",
				protocol == A_IMAP4 ? "imap":"nntp",
				tmp_ac_prefs.account_name ? tmp_ac_prefs.account_name : "(null)");
	
	prefs_set_data_from_dialog(basic_param);
	
	if (protocol == A_IMAP4 || protocol == A_NNTP) {
		new_id = g_strdup_printf("#%s/%s",
				protocol == A_IMAP4 ? "imap":"nntp",
				tmp_ac_prefs.account_name);
		if (old_id != NULL && new_id != NULL)
			prefs_filtering_rename_path(old_id, new_id);
		g_free(old_id);
		g_free(new_id);
	}
	
	return 0;
}

static gint prefs_receive_apply(void)
{
	prefs_set_data_from_dialog(receive_param);
	return 0;
}

static gint prefs_send_apply(void)
{
	prefs_set_data_from_dialog(send_param);
	return 0;
}

static gint prefs_compose_apply(void)
{
	prefs_set_data_from_dialog(compose_param);
	return 0;
}

static gint prefs_templates_apply(void)
{
	prefs_set_data_from_dialog(templates_param);
	return 0;
}

static gint prefs_privacy_apply(void)
{
	prefs_set_data_from_dialog(privacy_param);
	return 0;
}

#if USE_OPENSSL
static gint prefs_ssl_apply(void)
{
	prefs_set_data_from_dialog(ssl_param);
	return 0;
}
#endif

static gint prefs_advanced_apply(void)
{
	prefs_set_data_from_dialog(advanced_param);
	return 0;
}

static void basic_destroy_widget_func(PrefsPage *_page)
{
	/* BasicPage *page = (BasicPage *) _page; */
}

static void receive_destroy_widget_func(PrefsPage *_page)
{
	/* ReceivePage *page = (ReceivePage *) _page; */
}

static void send_destroy_widget_func(PrefsPage *_page)
{
	/* SendPage *page = (SendPage *) _page; */
}

static void compose_destroy_widget_func(PrefsPage *_page)
{
	/* ComposePage *page = (ComposePage *) _page; */
}

static void templates_destroy_widget_func(PrefsPage *_page)
{
	/* TemplatesPage *page = (TemplatesPage *) _page; */
}

static void privacy_destroy_widget_func(PrefsPage *_page)
{
	/* PrivacyPage *page = (PrivacyPage *) _page; */
}

#if USE_OPENSSL
static void ssl_destroy_widget_func(PrefsPage *_page)
{
	/* SSLPage *page = (SSLPage *) _page; */
}
#endif

static void advanced_destroy_widget_func(PrefsPage *_page)
{
	/* AdvancedPage *page = (AdvancedPage *) _page; */
}

static gboolean basic_can_close_func(PrefsPage *_page)
{	
	BasicPage *page = (BasicPage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_basic_apply() >= 0;
}

static gboolean receive_can_close_func(PrefsPage *_page)
{	
	ReceivePage *page = (ReceivePage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_receive_apply() >= 0;
}

static gboolean send_can_close_func(PrefsPage *_page)
{	
	SendPage *page = (SendPage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_send_apply() >= 0;
}

static gboolean compose_can_close_func(PrefsPage *_page)
{	
	ComposePage *page = (ComposePage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_compose_apply() >= 0;
}

static gboolean templates_can_close_func(PrefsPage *_page)
{
	TemplatesPage *page = (TemplatesPage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_templates_apply() >= 0;
}

static gboolean privacy_can_close_func(PrefsPage *_page)
{
	PrivacyPage *page = (PrivacyPage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_privacy_apply() >= 0;
}

#if USE_OPENSSL
static gboolean ssl_can_close_func(PrefsPage *_page)
{
	SSLPage *page = (SSLPage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_ssl_apply() >= 0;
}
#endif

static gboolean advanced_can_close_func(PrefsPage *_page)
{
	AdvancedPage *page = (AdvancedPage *) _page;

	if (!page->page.page_open)
		return TRUE;

	return prefs_advanced_apply() >= 0;
}

static void basic_save_func(PrefsPage *_page)
{
	BasicPage *page = (BasicPage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_basic_apply() >= 0)
		cancelled = FALSE;
}

static void receive_save_func(PrefsPage *_page)
{
	ReceivePage *page = (ReceivePage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_receive_apply() >= 0)
		cancelled = FALSE;
}

static void send_save_func(PrefsPage *_page)
{
	SendPage *page = (SendPage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_send_apply() >= 0)
		cancelled = FALSE;
}

static void compose_save_func(PrefsPage *_page)
{
	ComposePage *page = (ComposePage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_compose_apply() >= 0)
		cancelled = FALSE;
}

static void templates_save_func(PrefsPage *_page)
{
	TemplatesPage *page = (TemplatesPage *) _page;

	if (!page->page.page_open)
		return;

	quotefmt_check_new_msg_formats(tmp_ac_prefs.compose_with_format,
									tmp_ac_prefs.compose_subject_format,
									tmp_ac_prefs.compose_body_format);
	quotefmt_check_reply_formats(tmp_ac_prefs.reply_with_format,
									tmp_ac_prefs.reply_body_format);
	quotefmt_check_forward_formats(tmp_ac_prefs.forward_with_format,
									tmp_ac_prefs.forward_body_format);
	if (prefs_templates_apply() >= 0)
		cancelled = FALSE;
}

static void privacy_save_func(PrefsPage *_page)
{
	PrivacyPage *page = (PrivacyPage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_privacy_apply() >= 0)
		cancelled = FALSE;
}

#if USE_OPENSSL
static void ssl_save_func(PrefsPage *_page)
{
	SSLPage *page = (SSLPage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_ssl_apply() >= 0)
		cancelled = FALSE;
}
#endif

static void advanced_save_func(PrefsPage *_page)
{
	AdvancedPage *page = (AdvancedPage *) _page;

	if (!page->page.page_open)
		return;

	if (prefs_advanced_apply() >= 0)
		cancelled = FALSE;
}

static void register_basic_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Basic");
	path[2] = NULL;
        
	basic_page.page.path = path;
	basic_page.page.weight = 1000.0;
	basic_page.page.create_widget = basic_create_widget_func;
	basic_page.page.destroy_widget = basic_destroy_widget_func;
	basic_page.page.save_page = basic_save_func;
	basic_page.page.can_close = basic_can_close_func;

	prefs_account_register_page((PrefsPage *) &basic_page);
}

static void register_receive_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Receive");
	path[2] = NULL;
        
	receive_page.page.path = path;
	receive_page.page.weight = 1000.0;
	receive_page.page.create_widget = receive_create_widget_func;
	receive_page.page.destroy_widget = receive_destroy_widget_func;
	receive_page.page.save_page = receive_save_func;
	receive_page.page.can_close = receive_can_close_func;

	prefs_account_register_page((PrefsPage *) &receive_page);
}

static void register_send_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Send");
	path[2] = NULL;
        
	send_page.page.path = path;
	send_page.page.weight = 1000.0;
	send_page.page.create_widget = send_create_widget_func;
	send_page.page.destroy_widget = send_destroy_widget_func;
	send_page.page.save_page = send_save_func;
	send_page.page.can_close = send_can_close_func;

	prefs_account_register_page((PrefsPage *) &send_page);
}

static void register_compose_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Compose");
	path[2] = NULL;
        
	compose_page.page.path = path;
	compose_page.page.weight = 1000.0;
	compose_page.page.create_widget = compose_create_widget_func;
	compose_page.page.destroy_widget = compose_destroy_widget_func;
	compose_page.page.save_page = compose_save_func;
	compose_page.page.can_close = compose_can_close_func;

	prefs_account_register_page((PrefsPage *) &compose_page);
}

static void register_templates_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Templates");
	path[2] = NULL;
        
	templates_page.page.path = path;
	templates_page.page.weight = 1000.0;
	templates_page.page.create_widget = templates_create_widget_func;
	templates_page.page.destroy_widget = templates_destroy_widget_func;
	templates_page.page.save_page = templates_save_func;
	templates_page.page.can_close = templates_can_close_func;

	prefs_account_register_page((PrefsPage *) &templates_page);
}

static void register_privacy_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Privacy");
	path[2] = NULL;
        
	privacy_page.page.path = path;
	privacy_page.page.weight = 1000.0;
	privacy_page.page.create_widget = privacy_create_widget_func;
	privacy_page.page.destroy_widget = privacy_destroy_widget_func;
	privacy_page.page.save_page = privacy_save_func;
	privacy_page.page.can_close = privacy_can_close_func;

	prefs_account_register_page((PrefsPage *) &privacy_page);
}

#if USE_OPENSSL
static void register_ssl_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("SSL");
	path[2] = NULL;
        
	ssl_page.page.path = path;
	ssl_page.page.weight = 1000.0;
	ssl_page.page.create_widget = ssl_create_widget_func;
	ssl_page.page.destroy_widget = ssl_destroy_widget_func;
	ssl_page.page.save_page = ssl_save_func;
	ssl_page.page.can_close = ssl_can_close_func;

	prefs_account_register_page((PrefsPage *) &ssl_page);
}
#endif

static void register_advanced_page(void)
{
	static gchar *path[3];

	path[0] = _("Account");
	path[1] = _("Advanced");
	path[2] = NULL;
        
	advanced_page.page.path = path;
	advanced_page.page.weight = 1000.0;
	advanced_page.page.create_widget = advanced_create_widget_func;
	advanced_page.page.destroy_widget = advanced_destroy_widget_func;
	advanced_page.page.save_page = advanced_save_func;
	advanced_page.page.can_close = advanced_can_close_func;

	prefs_account_register_page((PrefsPage *) &advanced_page);
}

void prefs_account_init()
{
	register_basic_page();
	register_receive_page();
	register_send_page();
	register_compose_page();
	register_templates_page();
	register_privacy_page();
#if USE_OPENSSL
	register_ssl_page();
#endif
	register_advanced_page();
}

PrefsAccount *prefs_account_new(void)
{
	PrefsAccount *ac_prefs;

	ac_prefs = g_new0(PrefsAccount, 1);
	memset(&tmp_ac_prefs, 0, sizeof(PrefsAccount));
	prefs_set_default(basic_param);
	prefs_set_default(receive_param);
	prefs_set_default(send_param);
	prefs_set_default(compose_param);
	prefs_set_default(templates_param);
	prefs_set_default(privacy_param);
	prefs_set_default(ssl_param);
	prefs_set_default(advanced_param);
	*ac_prefs = tmp_ac_prefs;
	ac_prefs->account_id = prefs_account_get_new_id();

	ac_prefs->privacy_prefs = g_hash_table_new(g_str_hash, g_str_equal);

	return ac_prefs;
}

void prefs_account_read_config(PrefsAccount *ac_prefs, const gchar *label)
{
	const gchar *p = label;
	gchar *rcpath;
	gint id;
	gchar **strv, **cur;

	g_return_if_fail(ac_prefs != NULL);
	g_return_if_fail(label != NULL);

	memset(&tmp_ac_prefs, 0, sizeof(PrefsAccount));
	tmp_ac_prefs.privacy_prefs = ac_prefs->privacy_prefs;

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, ACCOUNT_RC, NULL);
	prefs_read_config(basic_param, label, rcpath, NULL);
	prefs_read_config(receive_param, label, rcpath, NULL);
	prefs_read_config(send_param, label, rcpath, NULL);
	prefs_read_config(compose_param, label, rcpath, NULL);
	prefs_read_config(templates_param, label, rcpath, NULL);
	prefs_read_config(privacy_param, label, rcpath, NULL);
	prefs_read_config(ssl_param, label, rcpath, NULL);
	prefs_read_config(advanced_param, label, rcpath, NULL);
	g_free(rcpath);

	*ac_prefs = tmp_ac_prefs;
	while (*p && !g_ascii_isdigit(*p)) p++;
	id = atoi(p);
	if (id < 0) g_warning("wrong account id: %d\n", id);
	ac_prefs->account_id = id;

	if (ac_prefs->protocol == A_APOP) {
		debug_print("converting protocol A_APOP to new prefs.\n");
		ac_prefs->protocol = A_POP3;
		ac_prefs->use_apop_auth = TRUE;
	}

	if (privacy_prefs != NULL) {
		strv = g_strsplit(privacy_prefs, ",", 0);
		for (cur = strv; *cur != NULL; cur++) {
			gchar *encvalue, *value;

			encvalue = strchr(*cur, '=');
			if (encvalue == NULL)
				continue;
			encvalue[0] = '\0';
			encvalue++;

			value = g_malloc0(strlen(encvalue));
			if (base64_decode(value, encvalue, strlen(encvalue)) > 0)
				g_hash_table_insert(ac_prefs->privacy_prefs, g_strdup(*cur), g_strdup(value));
			g_free(value);
		}
		g_strfreev(strv);
		g_free(privacy_prefs);
		privacy_prefs = NULL;
	}

	prefs_custom_header_read_config(ac_prefs);
}

static void create_privacy_prefs(gpointer key, gpointer _value, gpointer user_data)
{
	GString *str = (GString *) user_data;
	gchar *encvalue;
	gchar *value = (gchar *) _value;

	if (str->len > 0)
		g_string_append_c(str, ',');

	encvalue = g_malloc0(B64LEN(strlen(value)) + 1);
	base64_encode(encvalue, (gchar *) value, strlen(value));
	g_string_append_printf(str, "%s=%s", (gchar *) key, encvalue);
	g_free(encvalue);
}

#define WRITE_PARAM(PARAM_TABLE) \
		if (prefs_write_param(PARAM_TABLE, pfile->fp) < 0) { \
			g_warning("failed to write configuration to file\n"); \
			prefs_file_close_revert(pfile); \
			g_free(privacy_prefs); \
			privacy_prefs = NULL; \
		    	return; \
 		}

void prefs_account_write_config_all(GList *account_list)
{
	GList *cur;
	gchar *rcpath;
	PrefFile *pfile;

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, ACCOUNT_RC, NULL);
	if ((pfile = prefs_write_open(rcpath)) == NULL) {
		g_free(rcpath);
		return;
	}
	g_free(rcpath);

	for (cur = account_list; cur != NULL; cur = cur->next) {
		GString *str;

		tmp_ac_prefs = *(PrefsAccount *)cur->data;
		if (fprintf(pfile->fp, "[Account: %d]\n",
			    tmp_ac_prefs.account_id) <= 0)
			return;

		str = g_string_sized_new(32);
		g_hash_table_foreach(tmp_ac_prefs.privacy_prefs, create_privacy_prefs, str);
		privacy_prefs = str->str;		    
		g_string_free(str, FALSE);

		WRITE_PARAM(basic_param)
		WRITE_PARAM(receive_param)
		WRITE_PARAM(send_param)
		WRITE_PARAM(compose_param)
		WRITE_PARAM(templates_param)
		WRITE_PARAM(privacy_param)
		WRITE_PARAM(ssl_param)
		WRITE_PARAM(advanced_param)

		g_free(privacy_prefs);
		privacy_prefs = NULL;

		if (cur->next) {
			if (fputc('\n', pfile->fp) == EOF) {
				FILE_OP_ERROR(rcpath, "fputc");
				prefs_file_close_revert(pfile);
				return;
			}
		}
	}

	if (prefs_file_close(pfile) < 0)
		g_warning("failed to write configuration to file\n");
}
#undef WRITE_PARAM

static gboolean free_privacy_prefs(gpointer key, gpointer value, gpointer user_data)
{
	g_free(key);
	g_free(value);

	return TRUE;
}

void prefs_account_free(PrefsAccount *ac_prefs)
{
	if (!ac_prefs) return;

	g_hash_table_foreach_remove(ac_prefs->privacy_prefs, free_privacy_prefs, NULL);

	tmp_ac_prefs = *ac_prefs;
	prefs_free(basic_param);
	prefs_free(receive_param);
	prefs_free(send_param);
	prefs_free(compose_param);
	prefs_free(templates_param);
	prefs_free(privacy_param);
	prefs_free(ssl_param);
	prefs_free(advanced_param);
}

const gchar *prefs_account_get_privacy_prefs(PrefsAccount *account, gchar *id)
{
	return g_hash_table_lookup(account->privacy_prefs, id);
}

void prefs_account_set_privacy_prefs(PrefsAccount *account, gchar *id, gchar *new_value)
{
	gchar *orig_key = NULL, *value;

	if (g_hash_table_lookup_extended(account->privacy_prefs, id, (gpointer *)(gchar *) &orig_key, (gpointer *)(gchar *) &value)) {
		g_hash_table_remove(account->privacy_prefs, id);

		g_free(orig_key);
		g_free(value);
	}

	if (new_value != NULL)
		g_hash_table_insert(account->privacy_prefs, g_strdup(id), g_strdup(new_value));
}

static gint prefs_account_get_new_id(void)
{
	GList *ac_list;
	PrefsAccount *ac;
	static gint last_id = 0;

	for (ac_list = account_get_list(); ac_list != NULL;
	     ac_list = ac_list->next) {
		ac = (PrefsAccount *)ac_list->data;
		if (last_id < ac->account_id)
			last_id = ac->account_id;
	}

	return last_id + 1;
}

static void destroy_dialog(gpointer data)
{
	PrefsAccount *ac_prefs = (PrefsAccount *) data;
	if (!cancelled) {
		gboolean update_fld_list = FALSE;
		if (ac_prefs->protocol == A_IMAP4 && !new_account) {
			if ((&tmp_ac_prefs)->imap_subsonly != ac_prefs->imap_subsonly) {
				update_fld_list = TRUE;
			} 
		}
		*ac_prefs = tmp_ac_prefs;
		if (update_fld_list)
			folderview_rescan_tree(ac_prefs->folder, FALSE);
	} else /* the customhdr_list may have changed, update it anyway */
		ac_prefs->customhdr_list = (&tmp_ac_prefs)->customhdr_list;

	
	gtk_main_quit();
}

PrefsAccount *prefs_account_open(PrefsAccount *ac_prefs, gboolean *dirty)
{
	gchar *title;

	if (prefs_rc_is_readonly(ACCOUNT_RC))
		return ac_prefs;

	debug_print("Opening account preferences window...\n");

	inc_lock();

	cancelled = TRUE;

	if (!ac_prefs) {
		ac_prefs = prefs_account_new();
		new_account = TRUE;
	} else
		new_account = FALSE;

	if (new_account)
		title = g_strdup (_("Preferences for new account"));
	else
		title = g_strdup_printf (_("%s - Account preferences"),
				ac_prefs->account_name);

	prefswindow_open_full(title, prefs_pages, ac_prefs, destroy_dialog,
			&prefs_common.editaccountwin_width, &prefs_common.editaccountwin_height,
			TRUE);
	g_free(title);
	gtk_main();

	inc_unlock();

	if (!cancelled && dirty != NULL)
		*dirty = TRUE;
	if (cancelled && new_account) {
		prefs_account_free(ac_prefs);
		return NULL;
	} else 
		return ac_prefs;
}

static void crosspost_color_toggled(void)
{
	gboolean is_active;

	is_active = gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON(advanced_page.crosspost_checkbtn));
	gtk_widget_set_sensitive(advanced_page.crosspost_colormenu, is_active);
}

static void prefs_account_crosspost_set_data_from_colormenu(PrefParam *pparam)
{
	GtkWidget *menu;
	GtkWidget *menuitem;

	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(advanced_page.crosspost_colormenu));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	*((gint *)pparam->data) = GPOINTER_TO_INT
		(g_object_get_data(G_OBJECT(menuitem), "color"));
}

static void prefs_account_crosspost_set_colormenu(PrefParam *pparam)
{
	gint colorlabel = *((gint *)pparam->data);
	GtkOptionMenu *colormenu = GTK_OPTION_MENU(*pparam->widget);
	GtkWidget *menu;
	GtkWidget *menuitem;

	gtk_option_menu_set_history(colormenu, colorlabel);
	menu = gtk_option_menu_get_menu(colormenu);
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
}

static void pop_bfr_smtp_tm_set_sens(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive(send_page.pop_bfr_smtp_tm_entry, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(send_page.pop_bfr_smtp_checkbtn)));
	gtk_widget_set_sensitive(send_page.pop_auth_timeout_lbl, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(send_page.pop_bfr_smtp_checkbtn)));
	gtk_widget_set_sensitive(send_page.pop_auth_minutes_lbl, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(send_page.pop_bfr_smtp_checkbtn)));
}

static void prefs_account_select_folder_cb(GtkWidget *widget, gpointer data)
{
	FolderItem *item;
	gchar *id;

	item = foldersel_folder_sel(NULL, FOLDER_SEL_COPY, NULL);
	if (item && item->path) {
		id = folder_item_get_identifier(item);
		if (id) {
			gtk_entry_set_text(GTK_ENTRY(data), id);
			g_free(id);
		}
	}
}

static void prefs_account_sigfile_radiobtn_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(signature_browse_button), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(signature_edit_button), TRUE);
}

static void prefs_account_sigcmd_radiobtn_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(signature_browse_button), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(signature_edit_button), FALSE);
}

static void prefs_account_signature_browse_cb(GtkWidget *widget, gpointer data)
{
	gchar *filename;
	gchar *utf8_filename;

	filename = filesel_select_file_open(_("Select signature file"), NULL);
	if (!filename) return;

	utf8_filename = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
	if (!utf8_filename) {
		g_warning("prefs_account_signature_browse_cb(): failed to convert character set.");
		utf8_filename = g_strdup(filename);
	}
	gtk_entry_set_text(GTK_ENTRY(entry_sigpath), utf8_filename);
	g_free(utf8_filename);
}

static void prefs_account_signature_edit_cb(GtkWidget *widget, gpointer data)
{
	const gchar *sigpath = gtk_entry_get_text(GTK_ENTRY(data));
	if (!is_file_exist(sigpath))
		str_write_to_file(sigpath, "");
	open_txt_editor(sigpath, prefs_common.ext_editor_cmd);
}

static void prefs_account_edit_custom_header(void)
{
	prefs_custom_header_open(&tmp_ac_prefs);
}

static void prefs_account_enum_set_data_from_radiobtn(PrefParam *pparam)
{
	GtkRadioButton *radiobtn;
	GSList *group;

	radiobtn = GTK_RADIO_BUTTON (*pparam->widget);
	group = gtk_radio_button_get_group (radiobtn);
	while (group != NULL) {
		GtkToggleButton *btn = GTK_TOGGLE_BUTTON (group->data);
		if (gtk_toggle_button_get_active (btn)) {
			*((gint *)pparam->data) = GPOINTER_TO_INT
				(g_object_get_data (G_OBJECT (btn), MENU_VAL_ID));
			break;
		}
		group = group->next;
	}
}

static void prefs_account_enum_set_radiobtn(PrefParam *pparam)
{
	GtkRadioButton *radiobtn;
	GSList *group;
	gpointer data;

	data = GINT_TO_POINTER (*((gint *)pparam->data));
	radiobtn = GTK_RADIO_BUTTON (*pparam->widget);
	group = gtk_radio_button_get_group (radiobtn);
	while (group != NULL) {
		GtkToggleButton *btn = GTK_TOGGLE_BUTTON (group->data);
		gpointer data1 = g_object_get_data (G_OBJECT (btn), MENU_VAL_ID);
		if (data1 == data) {
			gtk_toggle_button_set_active (btn, TRUE);
			break;
		}
		group = group->next;
	}
}

static void prefs_account_protocol_set_data_from_optmenu(PrefParam *pparam)
{
	struct BasicProtocol *protocol_optmenu =
		(struct BasicProtocol *)*pparam->widget;
	GtkWidget *optmenu = protocol_optmenu->combobox;

	*((RecvProtocol *)pparam->data) =
		combobox_get_active_data(GTK_COMBO_BOX(optmenu));
}

static void prefs_account_protocol_set_optmenu(PrefParam *pparam)
{
	RecvProtocol protocol;
	struct BasicProtocol *protocol_optmenu =
		(struct BasicProtocol *)*pparam->widget;
	GtkWidget *optmenu = protocol_optmenu->combobox;
	GtkWidget *optlabel = protocol_optmenu->label;
	GtkWidget *descrlabel = protocol_optmenu->descrlabel;
	gchar *label = NULL;

	protocol = *((RecvProtocol *)pparam->data);

	/* Set combobox to correct value even if it will be hidden, so
	 * we won't break existing accounts when saving. */
	combobox_select_by_data(GTK_COMBO_BOX(optmenu), protocol);

	/* Set up widgets accordingly */
	if( new_account ) {
		gtk_label_set_text(GTK_LABEL(descrlabel), _("Protocol"));
		gtk_widget_hide(optlabel);
		gtk_widget_show(optmenu);
	} else {
		gtk_label_set_text(GTK_LABEL(descrlabel), _("Protocol:"));
		label = g_markup_printf_escaped("<b>%s</b>", protocol_names[protocol]);
		gtk_label_set_markup(GTK_LABEL(optlabel), label);
		g_free(label);
		gtk_widget_hide(optmenu);
		gtk_widget_show(optlabel);
#ifndef HAVE_LIBETPAN
		if (protocol == A_IMAP4) {
			gtk_widget_show(protocol_optmenu->no_imap_warn_icon);
			gtk_widget_show(protocol_optmenu->no_imap_warn_label);
		} else {
			gtk_widget_hide(protocol_optmenu->no_imap_warn_icon);
			gtk_widget_hide(protocol_optmenu->no_imap_warn_label);
		}
#endif
	}
}

static void prefs_account_imap_auth_type_set_data_from_optmenu(PrefParam *pparam)
{
	GtkWidget *menu;
	GtkWidget *menuitem;

	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(*pparam->widget));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	*((RecvProtocol *)pparam->data) = GPOINTER_TO_INT
		(g_object_get_data(G_OBJECT(menuitem), MENU_VAL_ID));
}

static void prefs_account_imap_auth_type_set_optmenu(PrefParam *pparam)
{
	IMAPAuthType type = *((IMAPAuthType *)pparam->data);
	GtkOptionMenu *optmenu = GTK_OPTION_MENU(*pparam->widget);
	GtkWidget *menu;
	GtkWidget *menuitem;

	switch (type) {
	case IMAP_AUTH_LOGIN:
		gtk_option_menu_set_history(optmenu, 1);
		break;
	case IMAP_AUTH_CRAM_MD5:
		gtk_option_menu_set_history(optmenu, 2);
		break;
	case IMAP_AUTH_ANON:
		gtk_option_menu_set_history(optmenu, 3);
		break;
	case IMAP_AUTH_GSSAPI:
		gtk_option_menu_set_history(optmenu, 4);
		break;
	case 0:
	default:
		gtk_option_menu_set_history(optmenu, 0);
	}

	menu = gtk_option_menu_get_menu(optmenu);
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
}

static void prefs_account_smtp_auth_type_set_data_from_optmenu(PrefParam *pparam)
{
	GtkWidget *menu;
	GtkWidget *menuitem;

	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(*pparam->widget));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	*((RecvProtocol *)pparam->data) = GPOINTER_TO_INT
		(g_object_get_data(G_OBJECT(menuitem), MENU_VAL_ID));
}

static void prefs_account_smtp_auth_type_set_optmenu(PrefParam *pparam)
{
	SMTPAuthType type = *((SMTPAuthType *)pparam->data);
	GtkOptionMenu *optmenu = GTK_OPTION_MENU(*pparam->widget);
	GtkWidget *menu;
	GtkWidget *menuitem;

	switch (type) {
	case SMTPAUTH_PLAIN:
		gtk_option_menu_set_history(optmenu, 1);
		break;
	case SMTPAUTH_LOGIN:
		gtk_option_menu_set_history(optmenu, 2);
		break;
	case SMTPAUTH_CRAM_MD5:
		gtk_option_menu_set_history(optmenu, 3);
		break;
	case SMTPAUTH_DIGEST_MD5:
		gtk_option_menu_set_history(optmenu, 4);
		break;
	case 0:
	default:
		gtk_option_menu_set_history(optmenu, 0);
	}

	menu = gtk_option_menu_get_menu(optmenu);
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
}

static void prefs_account_set_string_from_optmenu(PrefParam *pparam)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	gchar **str;

	g_return_if_fail(*pparam->widget != NULL);

	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(*pparam->widget));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	if (menuitem == NULL)
		return;

	str = (gchar **) pparam->data;
        g_free(*str);
	*str = g_strdup(g_object_get_data(G_OBJECT(menuitem), MENU_VAL_ID));
}

static void prefs_account_set_optmenu_from_string(PrefParam *pparam)
{
	GtkWidget *optionmenu;
	GtkWidget *menu;
	gboolean found = FALSE;
	GList *children, *cur;
	gchar *prefsid;
	guint i = 0;

	g_return_if_fail(*pparam->widget != NULL);

	prefsid = *((gchar **) pparam->data);
	if (prefsid == NULL)
		return;

	optionmenu = *pparam->widget;
	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(optionmenu));
	children = gtk_container_children(GTK_CONTAINER(menu));
	for (cur = children; cur != NULL; cur = g_list_next(cur)) {
		GtkWidget *item = (GtkWidget *) cur->data;
		gchar *id;

		id = g_object_get_data(G_OBJECT(item), MENU_VAL_ID);
		if (id != NULL && strcmp(id, prefsid) == 0) {
			found = TRUE;
			gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), i);
			privacy_system_activated(GTK_MENU_ITEM(item));
		}
		i++;
	}

	if (!found) {
		gchar *name;
		GtkWidget *menuitem;

		name = g_strdup_printf(_("%s (plugin not loaded)"), prefsid);
		menuitem = gtk_menu_item_new_with_label(name);
		gtk_widget_show(menuitem);
		g_object_set_data(G_OBJECT(menuitem), MENU_VAL_ID, prefsid);
		g_object_set_data(G_OBJECT(menuitem), MENU_VAL_DATA, GINT_TO_POINTER(FALSE));
		gtk_menu_append(GTK_MENU(menu), menuitem);
		g_free(name);

		gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), i);
		privacy_system_activated(GTK_MENU_ITEM(menuitem));
		g_signal_connect(G_OBJECT(menuitem), "activate",
				 G_CALLBACK(privacy_system_activated),
				 NULL);
	}

	g_list_free(children);
}

static void prefs_account_protocol_changed(GtkComboBox *combobox, gpointer data)
{
	RecvProtocol protocol;
	struct BasicProtocol *protocol_optmenu = (struct BasicProtocol *)basic_page.protocol_optmenu;

	protocol = combobox_get_active_data(combobox);

	gtk_widget_hide(protocol_optmenu->no_imap_warn_icon);
	gtk_widget_hide(protocol_optmenu->no_imap_warn_label);
	switch(protocol) {
	case A_NNTP:
		gtk_widget_hide(protocol_optmenu->no_imap_warn_icon);
		gtk_widget_hide(protocol_optmenu->no_imap_warn_label);
		gtk_widget_show(basic_page.nntpserv_label);
		gtk_widget_show(basic_page.nntpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   0, VSPACING_NARROW);

		gtk_widget_set_sensitive(basic_page.nntpauth_checkbtn, TRUE);
		gtk_widget_show(basic_page.nntpauth_checkbtn);

		gtk_widget_set_sensitive(basic_page.nntpauth_onconnect_checkbtn, TRUE);
		gtk_widget_show(basic_page.nntpauth_onconnect_checkbtn);

  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   1, VSPACING_NARROW);
		gtk_widget_hide(basic_page.recvserv_label);
		gtk_widget_hide(basic_page.recvserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   2, 0);
		gtk_widget_show(basic_page.smtpserv_label);
		gtk_widget_show(basic_page.smtpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   4, VSPACING_NARROW);
		gtk_widget_hide(basic_page.localmbox_label);
		gtk_widget_hide(basic_page.localmbox_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   3, 0);
		gtk_widget_hide(basic_page.mailcmd_label);
		gtk_widget_hide(basic_page.mailcmd_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   6, 0);
		gtk_widget_hide(basic_page.mailcmd_checkbtn);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   5, 0);
		gtk_widget_show(basic_page.uid_label);
		gtk_widget_show(basic_page.pass_label);
		gtk_widget_show(basic_page.uid_entry);
		gtk_widget_show(basic_page.pass_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   7, VSPACING_NARROW);

		gtk_widget_set_sensitive(basic_page.uid_label,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_label, TRUE);
		gtk_widget_set_sensitive(basic_page.uid_entry,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_entry, TRUE);

		/* update userid/passwd sensitive state */

		prefs_account_nntpauth_toggled
			(GTK_TOGGLE_BUTTON(basic_page.nntpauth_checkbtn), NULL);
		gtk_widget_hide(receive_page.pop3_frame);
		gtk_widget_hide(receive_page.imap_frame);
		gtk_widget_hide(receive_page.local_frame);
		gtk_widget_show(receive_page.frame_maxarticle);
		gtk_widget_set_sensitive(receive_page.filter_on_recv_checkbtn, TRUE);
		gtk_widget_set_sensitive(receive_page.recvatgetall_checkbtn, TRUE);
		/* update pop_before_smtp sensitivity */
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(send_page.pop_bfr_smtp_checkbtn), FALSE);
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_checkbtn, FALSE);
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_tm_entry, FALSE);
		
		if (!tmp_ac_prefs.account_name) {
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.filter_on_recv_checkbtn), 
				TRUE);
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.recvatgetall_checkbtn),
				 FALSE);
		}

#if USE_OPENSSL
		gtk_widget_hide(ssl_page.pop_frame);
		gtk_widget_hide(ssl_page.imap_frame);
		gtk_widget_show(ssl_page.nntp_frame);
		gtk_widget_show(ssl_page.send_frame);
#endif
		gtk_widget_hide(advanced_page.popport_hbox);
		gtk_widget_hide(advanced_page.imapport_hbox);
		gtk_widget_show(advanced_page.nntpport_hbox);
		gtk_widget_show(advanced_page.crosspost_checkbtn);
		gtk_widget_show(advanced_page.crosspost_colormenu);
		gtk_widget_hide(advanced_page.tunnelcmd_checkbtn);
		gtk_widget_hide(advanced_page.tunnelcmd_entry);
		gtk_widget_hide(receive_page.imapdir_label);
		gtk_widget_hide(receive_page.imapdir_entry);
		gtk_widget_hide(receive_page.subsonly_checkbtn);
		gtk_widget_hide(receive_page.low_bandwidth_checkbtn);
		break;
	case A_LOCAL:
		gtk_widget_hide(protocol_optmenu->no_imap_warn_icon);
		gtk_widget_hide(protocol_optmenu->no_imap_warn_label);
		gtk_widget_hide(basic_page.nntpserv_label);
		gtk_widget_hide(basic_page.nntpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   0, 0);
		gtk_widget_set_sensitive(basic_page.nntpauth_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_checkbtn);

		gtk_widget_set_sensitive(basic_page.nntpauth_onconnect_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_onconnect_checkbtn);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   1, 0);
		gtk_widget_hide(basic_page.recvserv_label);
		gtk_widget_hide(basic_page.recvserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   2, 0);
		gtk_widget_show(basic_page.smtpserv_label);
		gtk_widget_show(basic_page.smtpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   4, VSPACING_NARROW);
		gtk_widget_show(basic_page.localmbox_label);
		gtk_widget_show(basic_page.localmbox_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   3, VSPACING_NARROW);
		gtk_widget_show(basic_page.mailcmd_label);
		gtk_widget_show(basic_page.mailcmd_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   6, VSPACING_NARROW);
		gtk_widget_show(basic_page.mailcmd_checkbtn);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   5, VSPACING_NARROW);
		gtk_widget_hide(basic_page.uid_label);
		gtk_widget_hide(basic_page.pass_label);
		gtk_widget_hide(basic_page.uid_entry);
		gtk_widget_hide(basic_page.pass_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   7, 0);

		gtk_widget_set_sensitive(basic_page.uid_label,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_label, TRUE);
		gtk_widget_set_sensitive(basic_page.uid_entry,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_entry, TRUE);
		gtk_widget_hide(receive_page.pop3_frame);
		gtk_widget_hide(receive_page.imap_frame);
		gtk_widget_show(receive_page.local_frame);
		gtk_widget_hide(receive_page.frame_maxarticle);
		gtk_widget_set_sensitive(receive_page.filter_on_recv_checkbtn, TRUE);
		gtk_widget_set_sensitive(receive_page.recvatgetall_checkbtn, TRUE);
		prefs_account_mailcmd_toggled
			(GTK_TOGGLE_BUTTON(basic_page.mailcmd_checkbtn), NULL);

		/* update pop_before_smtp sensitivity */
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(send_page.pop_bfr_smtp_checkbtn), FALSE);
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_checkbtn, FALSE);
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_tm_entry, FALSE);

		if (!tmp_ac_prefs.account_name) {
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.filter_on_recv_checkbtn), 
				TRUE);
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.recvatgetall_checkbtn),
				 TRUE);
		}

#if USE_OPENSSL
		gtk_widget_hide(ssl_page.pop_frame);
		gtk_widget_hide(ssl_page.imap_frame);
		gtk_widget_hide(ssl_page.nntp_frame);
		gtk_widget_show(ssl_page.send_frame);
#endif
		gtk_widget_hide(advanced_page.popport_hbox);
		gtk_widget_hide(advanced_page.imapport_hbox);
		gtk_widget_hide(advanced_page.nntpport_hbox);
		gtk_widget_hide(advanced_page.crosspost_checkbtn);
		gtk_widget_hide(advanced_page.crosspost_colormenu);
		gtk_widget_hide(advanced_page.tunnelcmd_checkbtn);
		gtk_widget_hide(advanced_page.tunnelcmd_entry);
		gtk_widget_hide(receive_page.imapdir_label);
		gtk_widget_hide(receive_page.imapdir_entry);
		gtk_widget_hide(receive_page.subsonly_checkbtn);
		gtk_widget_hide(receive_page.low_bandwidth_checkbtn);
		break;
	case A_IMAP4:
#ifndef HAVE_LIBETPAN
		gtk_widget_show(protocol_optmenu->no_imap_warn_icon);
		gtk_widget_show(protocol_optmenu->no_imap_warn_label);
#endif
		gtk_widget_hide(basic_page.nntpserv_label);
		gtk_widget_hide(basic_page.nntpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   0, 0);
		gtk_widget_set_sensitive(basic_page.nntpauth_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_checkbtn);

		gtk_widget_set_sensitive(basic_page.nntpauth_onconnect_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_onconnect_checkbtn);

  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   1, 0);
		gtk_widget_set_sensitive(basic_page.recvserv_label, TRUE);
		gtk_widget_set_sensitive(basic_page.recvserv_entry, TRUE);
		gtk_widget_show(basic_page.recvserv_label);
		gtk_widget_show(basic_page.recvserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   2, VSPACING_NARROW);
		gtk_widget_show(basic_page.smtpserv_label);
		gtk_widget_show(basic_page.smtpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   4, VSPACING_NARROW);
		gtk_widget_hide(basic_page.localmbox_label);
		gtk_widget_hide(basic_page.localmbox_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   3, 0);
		gtk_widget_hide(basic_page.mailcmd_label);
		gtk_widget_hide(basic_page.mailcmd_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   6, 0);
		gtk_widget_hide(basic_page.mailcmd_checkbtn);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   5, 0);
		gtk_widget_show(basic_page.uid_label);
		gtk_widget_show(basic_page.pass_label);
		gtk_widget_show(basic_page.uid_entry);
		gtk_widget_show(basic_page.pass_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   7, VSPACING_NARROW);

		gtk_widget_set_sensitive(basic_page.uid_label,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_label, TRUE);
		gtk_widget_set_sensitive(basic_page.uid_entry,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_entry, TRUE);
		gtk_widget_hide(receive_page.pop3_frame);
		gtk_widget_show(receive_page.imap_frame);
		gtk_widget_hide(receive_page.local_frame);
		gtk_widget_hide(receive_page.frame_maxarticle);
		gtk_widget_set_sensitive(receive_page.filter_on_recv_checkbtn, TRUE);
		gtk_widget_set_sensitive(receive_page.recvatgetall_checkbtn, TRUE);
		gtk_widget_set_sensitive(basic_page.smtpserv_entry, TRUE);
		gtk_widget_set_sensitive(basic_page.smtpserv_label, TRUE);

		/* update pop_before_smtp sensitivity */
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(send_page.pop_bfr_smtp_checkbtn), FALSE);
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_checkbtn, FALSE);
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_tm_entry, FALSE);

		if (!tmp_ac_prefs.account_name) {
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.filter_on_recv_checkbtn), 
				TRUE);
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.recvatgetall_checkbtn),
				 FALSE);
		}

#if USE_OPENSSL
		gtk_widget_hide(ssl_page.pop_frame);
		gtk_widget_show(ssl_page.imap_frame);
		gtk_widget_hide(ssl_page.nntp_frame);
		gtk_widget_show(ssl_page.send_frame);
#endif
		gtk_widget_hide(advanced_page.popport_hbox);
		gtk_widget_show(advanced_page.imapport_hbox);
		gtk_widget_hide(advanced_page.nntpport_hbox);
		gtk_widget_hide(advanced_page.crosspost_checkbtn);
		gtk_widget_hide(advanced_page.crosspost_colormenu);
		gtk_widget_show(advanced_page.tunnelcmd_checkbtn);
		gtk_widget_show(advanced_page.tunnelcmd_entry);
		gtk_widget_show(receive_page.imapdir_label);
		gtk_widget_show(receive_page.imapdir_entry);
		gtk_widget_show(receive_page.subsonly_checkbtn);
		gtk_widget_show(receive_page.low_bandwidth_checkbtn);
		break;
	case A_NONE:
		gtk_widget_hide(protocol_optmenu->no_imap_warn_icon);
		gtk_widget_hide(protocol_optmenu->no_imap_warn_label);
		gtk_widget_hide(basic_page.nntpserv_label);
		gtk_widget_hide(basic_page.nntpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   0, 0);
		gtk_widget_set_sensitive(basic_page.nntpauth_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_checkbtn);

		gtk_widget_set_sensitive(basic_page.nntpauth_onconnect_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_onconnect_checkbtn);

  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   1, 0);
		gtk_widget_set_sensitive(basic_page.recvserv_label, FALSE);
		gtk_widget_set_sensitive(basic_page.recvserv_entry, FALSE);
		gtk_widget_hide(basic_page.recvserv_label);
		gtk_widget_hide(basic_page.recvserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   2, VSPACING_NARROW);
		gtk_widget_show(basic_page.smtpserv_label);
		gtk_widget_show(basic_page.smtpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   4, VSPACING_NARROW);
		gtk_widget_hide(basic_page.localmbox_label);
		gtk_widget_hide(basic_page.localmbox_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   3, 0);
		gtk_widget_hide(basic_page.mailcmd_label);
		gtk_widget_hide(basic_page.mailcmd_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   6, 0);
		gtk_widget_hide(basic_page.mailcmd_checkbtn);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   5, 0);
		gtk_widget_hide(basic_page.uid_label);
		gtk_widget_hide(basic_page.pass_label);
		gtk_widget_hide(basic_page.uid_entry);
		gtk_widget_hide(basic_page.pass_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   7, VSPACING_NARROW);

		gtk_widget_set_sensitive(basic_page.uid_label,  FALSE);
		gtk_widget_set_sensitive(basic_page.pass_label, FALSE);
		gtk_widget_set_sensitive(basic_page.uid_entry,  FALSE);
		gtk_widget_set_sensitive(basic_page.pass_entry, FALSE);
		gtk_widget_set_sensitive(receive_page.pop3_frame, FALSE);
		gtk_widget_hide(receive_page.pop3_frame);
		gtk_widget_hide(receive_page.imap_frame);
		gtk_widget_hide(receive_page.local_frame);
		gtk_widget_hide(receive_page.frame_maxarticle);
		gtk_widget_set_sensitive(receive_page.filter_on_recv_checkbtn, FALSE);
		gtk_widget_set_sensitive(receive_page.recvatgetall_checkbtn, FALSE);

		gtk_widget_set_sensitive(basic_page.smtpserv_entry, TRUE);
		gtk_widget_set_sensitive(basic_page.smtpserv_label, TRUE);

		/* update pop_before_smtp sensitivity */
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_checkbtn, FALSE);
		pop_bfr_smtp_tm_set_sens(NULL, NULL);
	
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(receive_page.filter_on_recv_checkbtn), FALSE);
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(receive_page.recvatgetall_checkbtn), FALSE);

#if USE_OPENSSL
		gtk_widget_hide(ssl_page.pop_frame);
		gtk_widget_hide(ssl_page.imap_frame);
		gtk_widget_hide(ssl_page.nntp_frame);
		gtk_widget_show(ssl_page.send_frame);
#endif
		gtk_widget_hide(advanced_page.popport_hbox);
		gtk_widget_hide(advanced_page.imapport_hbox);
		gtk_widget_hide(advanced_page.nntpport_hbox);
		gtk_widget_hide(advanced_page.crosspost_checkbtn);
		gtk_widget_hide(advanced_page.crosspost_colormenu);
		gtk_widget_hide(advanced_page.tunnelcmd_checkbtn);
		gtk_widget_hide(advanced_page.tunnelcmd_entry);
		gtk_widget_hide(receive_page.imapdir_label);
		gtk_widget_hide(receive_page.imapdir_entry);
		gtk_widget_hide(receive_page.subsonly_checkbtn);
		gtk_widget_hide(receive_page.low_bandwidth_checkbtn);
		break;
	case A_POP3:
	default:
		gtk_widget_hide(protocol_optmenu->no_imap_warn_icon);
		gtk_widget_hide(protocol_optmenu->no_imap_warn_label);
		gtk_widget_hide(basic_page.nntpserv_label);
		gtk_widget_hide(basic_page.nntpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   0, 0);
		gtk_widget_set_sensitive(basic_page.nntpauth_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_checkbtn);

		gtk_widget_set_sensitive(basic_page.nntpauth_onconnect_checkbtn, FALSE);
		gtk_widget_hide(basic_page.nntpauth_onconnect_checkbtn);

  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   1, 0);
		gtk_widget_set_sensitive(basic_page.recvserv_label, TRUE);
		gtk_widget_set_sensitive(basic_page.recvserv_entry, TRUE);
		gtk_widget_show(basic_page.recvserv_label);
		gtk_widget_show(basic_page.recvserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   2, VSPACING_NARROW);
		gtk_widget_show(basic_page.smtpserv_label);
		gtk_widget_show(basic_page.smtpserv_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   4, VSPACING_NARROW);
		gtk_widget_hide(basic_page.localmbox_label);
		gtk_widget_hide(basic_page.localmbox_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   3, 0);
		gtk_widget_hide(basic_page.mailcmd_label);
		gtk_widget_hide(basic_page.mailcmd_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   6, 0);
		gtk_widget_hide(basic_page.mailcmd_checkbtn);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   5, 0);
		gtk_widget_show(basic_page.uid_label);
		gtk_widget_show(basic_page.pass_label);
		gtk_widget_show(basic_page.uid_entry);
		gtk_widget_show(basic_page.pass_entry);
  		gtk_table_set_row_spacing (GTK_TABLE (basic_page.serv_table),
					   7, VSPACING_NARROW);

		gtk_widget_set_sensitive(basic_page.uid_label,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_label, TRUE);
		gtk_widget_set_sensitive(basic_page.uid_entry,  TRUE);
		gtk_widget_set_sensitive(basic_page.pass_entry, TRUE);
		gtk_widget_set_sensitive(receive_page.pop3_frame, TRUE);
		gtk_widget_show(receive_page.pop3_frame);
		gtk_widget_hide(receive_page.imap_frame);
		gtk_widget_hide(receive_page.local_frame);
		gtk_widget_hide(receive_page.frame_maxarticle);
		gtk_widget_set_sensitive(receive_page.filter_on_recv_checkbtn, TRUE);
		gtk_widget_set_sensitive(receive_page.recvatgetall_checkbtn, TRUE);

		gtk_widget_set_sensitive(basic_page.smtpserv_entry, TRUE);
		gtk_widget_set_sensitive(basic_page.smtpserv_label, TRUE);

		/* update pop_before_smtp sensitivity */
		gtk_widget_set_sensitive(send_page.pop_bfr_smtp_checkbtn, TRUE);
		pop_bfr_smtp_tm_set_sens(NULL, NULL);
		
		if (!tmp_ac_prefs.account_name) {
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.filter_on_recv_checkbtn), 
				TRUE);
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON(receive_page.recvatgetall_checkbtn),
				 TRUE);
		}

#if USE_OPENSSL
		gtk_widget_show(ssl_page.pop_frame);
		gtk_widget_hide(ssl_page.imap_frame);
		gtk_widget_hide(ssl_page.nntp_frame);
		gtk_widget_show(ssl_page.send_frame);
#endif
		gtk_widget_show(advanced_page.popport_hbox);
		gtk_widget_hide(advanced_page.imapport_hbox);
		gtk_widget_hide(advanced_page.nntpport_hbox);
		gtk_widget_hide(advanced_page.crosspost_checkbtn);
		gtk_widget_hide(advanced_page.crosspost_colormenu);
		gtk_widget_hide(advanced_page.tunnelcmd_checkbtn);
		gtk_widget_hide(advanced_page.tunnelcmd_entry);
		gtk_widget_hide(receive_page.imapdir_label);
		gtk_widget_hide(receive_page.imapdir_entry);
		gtk_widget_hide(receive_page.subsonly_checkbtn);
		gtk_widget_hide(receive_page.low_bandwidth_checkbtn);
		break;
	}

	gtk_widget_queue_resize(basic_page.serv_frame);
}

static void prefs_account_nntpauth_toggled(GtkToggleButton *button,
					   gpointer user_data)
{
	gboolean auth;

	if (!GTK_WIDGET_SENSITIVE (GTK_WIDGET (button)))
		return;
	auth = gtk_toggle_button_get_active (button);
	gtk_widget_set_sensitive(basic_page.uid_label,  auth);
	gtk_widget_set_sensitive(basic_page.pass_label, auth);
	gtk_widget_set_sensitive(basic_page.uid_entry,  auth);
	gtk_widget_set_sensitive(basic_page.pass_entry, auth);
	gtk_widget_set_sensitive(basic_page.nntpauth_onconnect_checkbtn, auth);
}

static void prefs_account_mailcmd_toggled(GtkToggleButton *button,
					  gpointer user_data)
{
	gboolean use_mailcmd;

	use_mailcmd = gtk_toggle_button_get_active (button);

	gtk_widget_set_sensitive(basic_page.mailcmd_entry,  use_mailcmd);
	gtk_widget_set_sensitive(basic_page.mailcmd_label, use_mailcmd);
	gtk_widget_set_sensitive(basic_page.smtpserv_entry, !use_mailcmd);
	gtk_widget_set_sensitive(basic_page.smtpserv_label, !use_mailcmd);
	gtk_widget_set_sensitive(basic_page.uid_entry,  !use_mailcmd);
	gtk_widget_set_sensitive(basic_page.pass_entry, !use_mailcmd);
}

#if USE_ASPELL
static void prefs_account_compose_default_dictionary_set_string_from_optmenu
							(PrefParam *pparam)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	gchar **str;

	g_return_if_fail(*pparam->widget != NULL);

	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(*pparam->widget));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	if (menuitem == NULL)
		return;

	str = (gchar **) pparam->data;
	g_free(*str);
	*str = gtkaspell_get_dictionary_menu_active_item(menu);
}

static void prefs_account_compose_default_dictionary_set_optmenu_from_string
							(PrefParam *pparam)
{
	GtkWidget *optionmenu;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gchar *dictionary;

	g_return_if_fail(*pparam->widget != NULL);

	dictionary = *((gchar **) pparam->data);
	if (dictionary == NULL)
		return;

	optionmenu = *pparam->widget;
	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(optionmenu));
	if (dictionary)
		gtkaspell_set_dictionary_menu_active_item(optionmenu, dictionary);
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
}
#endif

void prefs_account_register_page(PrefsPage *page)
{
	prefs_pages = g_slist_append(prefs_pages, page);
	
}

void prefs_account_unregister_page(PrefsPage *page)
{
	prefs_pages = g_slist_remove(prefs_pages, page);
}

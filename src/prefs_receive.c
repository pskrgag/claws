/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2005-2006 Colin Leroy <colin@colino.net> & The Sylpheed-Claws Team
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

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "prefs_common.h"
#include "prefs_gtk.h"
#include "inc.h"

#include "gtk/gtkutils.h"
#include "gtk/prefswindow.h"
#include "gtk/menu.h"

#include "manage_window.h"

typedef struct _ReceivePage
{
	PrefsPage page;

	GtkWidget *window;

	GtkWidget *checkbtn_incext;
	GtkWidget *entry_incext;
	GtkWidget *checkbtn_autochk;
	GtkWidget *spinbtn_autochk;
	GtkWidget *checkbtn_chkonstartup;
	GtkWidget *checkbtn_openinbox;
	GtkWidget *checkbtn_scan_after_inc;
	GtkWidget *checkbtn_newmail_auto;
	GtkWidget *checkbtn_newmail_manu;
	GtkWidget *entry_newmail_notify_cmd;
	GtkWidget *optmenu_recvdialog;
	GtkWidget *checkbtn_no_recv_err_panel;
	GtkWidget *checkbtn_close_recv_dialog;
	GtkWidget *hbox_newmail_notify;
} ReceivePage;

ReceivePage *prefs_receive;

static void prefs_common_recv_dialog_newmail_notify_toggle_cb(GtkWidget *w, gpointer data)
{
	gboolean toggled;

	toggled = gtk_toggle_button_get_active
			(GTK_TOGGLE_BUTTON(prefs_receive->checkbtn_newmail_manu)) ||
		  gtk_toggle_button_get_active
			(GTK_TOGGLE_BUTTON(prefs_receive->checkbtn_newmail_auto));
	gtk_widget_set_sensitive(prefs_receive->hbox_newmail_notify, toggled);
}


void prefs_receive_create_widget(PrefsPage *_page, GtkWindow *window, 
			       	  gpointer data)
{
	ReceivePage *prefs_receive = (ReceivePage *) _page;
	
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *checkbtn_incext;
	GtkWidget *hbox;
	GtkWidget *label_incext;
	GtkWidget *entry_incext;

	GtkWidget *hbox_autochk;
	GtkWidget *checkbtn_autochk;
	GtkWidget *label_autochk1;
	GtkObject *spinbtn_autochk_adj;
	GtkWidget *spinbtn_autochk;
	GtkWidget *label_autochk2;
	GtkWidget *checkbtn_chkonstartup;
	GtkWidget *checkbtn_openinbox;
	GtkWidget *checkbtn_scan_after_inc;

	GtkWidget *frame_newmail;
	GtkWidget *hbox_newmail_notify;
	GtkWidget *checkbtn_newmail_auto;
	GtkWidget *checkbtn_newmail_manu;
	GtkWidget *entry_newmail_notify_cmd;
	GtkWidget *label_newmail_notify_cmd;

	GtkWidget *table;
	GtkWidget *label_recvdialog;
	GtkWidget *menu;
	GtkWidget *menuitem;
	GtkWidget *optmenu_recvdialog;
	GtkWidget *checkbtn_no_recv_err_panel;
	GtkWidget *checkbtn_close_recv_dialog;

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox2 = gtk_vbox_new (FALSE, VSPACING_NARROW);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

	/* Use of external incorporation program */
	PACK_CHECK_BUTTON (vbox2, checkbtn_incext,
			   _("Use external program for receiving mail"));

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
	SET_TOGGLE_SENSITIVITY (checkbtn_incext, hbox);

	label_incext = gtk_label_new (_("Command"));
	gtk_widget_show (label_incext);
	gtk_box_pack_start (GTK_BOX (hbox), label_incext, FALSE, FALSE, 0);

	entry_incext = gtk_entry_new ();
	gtk_widget_show (entry_incext);
	gtk_box_pack_start (GTK_BOX (hbox), entry_incext, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);

	/* Auto-checking */
	hbox_autochk = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_autochk);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox_autochk, FALSE, FALSE, 0);

	PACK_CHECK_BUTTON (hbox_autochk, checkbtn_autochk,
			   _("Automatically check for new mail"));

	label_autochk1 = gtk_label_new (_("every"));
	gtk_widget_show (label_autochk1);
	gtk_box_pack_start (GTK_BOX (hbox_autochk), label_autochk1, FALSE, FALSE, 0);

	spinbtn_autochk_adj = gtk_adjustment_new (5, 1, 100, 1, 10, 10);
	spinbtn_autochk = gtk_spin_button_new
		(GTK_ADJUSTMENT (spinbtn_autochk_adj), 1, 0);
	gtk_widget_show (spinbtn_autochk);
	gtk_box_pack_start (GTK_BOX (hbox_autochk), spinbtn_autochk, FALSE, FALSE, 0);
	gtk_widget_set_size_request (spinbtn_autochk, 64, -1);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbtn_autochk), TRUE);

	label_autochk2 = gtk_label_new (_("minute(s)"));
	gtk_widget_show (label_autochk2);
	gtk_box_pack_start (GTK_BOX (hbox_autochk), label_autochk2, FALSE, FALSE, 0);

	SET_TOGGLE_SENSITIVITY(checkbtn_autochk, label_autochk1);
	SET_TOGGLE_SENSITIVITY(checkbtn_autochk, spinbtn_autochk);
	SET_TOGGLE_SENSITIVITY(checkbtn_autochk, label_autochk2);

	PACK_CHECK_BUTTON (vbox2, checkbtn_chkonstartup,
			   _("Check for new mail on startup"));
	PACK_CHECK_BUTTON (vbox2, checkbtn_openinbox,
		 	   _("Go to inbox after receiving new mail"));
	PACK_CHECK_BUTTON (vbox2, checkbtn_scan_after_inc,
			   _("Update all local folders after receiving mail"));

	/* receive dialog */
	table = gtk_table_new(1, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add (GTK_CONTAINER (vbox2), table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	label_recvdialog = gtk_label_new (_("Show receive dialog"));
	gtk_misc_set_alignment(GTK_MISC(label_recvdialog), 0, 0.5);
	gtk_widget_show (label_recvdialog);
	gtk_table_attach(GTK_TABLE(table), label_recvdialog, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);

	optmenu_recvdialog = gtk_option_menu_new ();
	gtk_widget_show (optmenu_recvdialog);
	gtk_table_attach(GTK_TABLE(table), optmenu_recvdialog, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 0, 0);

	menu = gtk_menu_new ();
	MENUITEM_ADD (menu, menuitem, _("Always"), RECV_DIALOG_ALWAYS);
	MENUITEM_ADD (menu, menuitem, _("Only on manual receiving"),
		      RECV_DIALOG_MANUAL);
	MENUITEM_ADD (menu, menuitem, _("Never"), RECV_DIALOG_NEVER);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (optmenu_recvdialog), menu);

	PACK_CHECK_BUTTON (vbox2, checkbtn_no_recv_err_panel,
			   _("Don't popup error dialog on receive error"));

	PACK_CHECK_BUTTON (vbox2, checkbtn_close_recv_dialog,
			   _("Close receive dialog when finished"));
	
	PACK_FRAME(vbox1, frame_newmail, _("Run command when new mail "
					   "arrives"));
	vbox2 = gtk_vbox_new (FALSE, VSPACING_NARROW);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (frame_newmail), vbox2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 8);

	hbox = gtk_hbox_new (TRUE, 8);
	gtk_widget_show (hbox);
	PACK_CHECK_BUTTON (hbox, checkbtn_newmail_auto,
			   _("after autochecking"));
	PACK_CHECK_BUTTON (hbox, checkbtn_newmail_manu,
			   _("after manual checking"));
	gtk_box_pack_start (GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	hbox_newmail_notify = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_newmail_notify);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox_newmail_notify, FALSE, 
			    FALSE, 0);

	label_newmail_notify_cmd = gtk_label_new (_("Command to execute:\n"
						    "(use %d as number of new "
						    "mails)"));
	gtk_label_set_justify(GTK_LABEL(label_newmail_notify_cmd), 
			      GTK_JUSTIFY_RIGHT);
	gtk_widget_show (label_newmail_notify_cmd);
	gtk_box_pack_start (GTK_BOX (hbox_newmail_notify), 
			    label_newmail_notify_cmd, FALSE, FALSE, 0);

	entry_newmail_notify_cmd = gtk_entry_new ();
	gtk_widget_show (entry_newmail_notify_cmd);
	gtk_box_pack_start (GTK_BOX (hbox_newmail_notify), 
			    entry_newmail_notify_cmd, TRUE, TRUE, 0);

	gtk_widget_set_sensitive(hbox_newmail_notify, 
				 prefs_common.newmail_notify_auto || 
				 prefs_common.newmail_notify_manu);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_incext),
		prefs_common.use_extinc);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_no_recv_err_panel),
		prefs_common.no_recv_err_panel);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_close_recv_dialog),
		prefs_common.close_recv_dialog);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_chkonstartup),
		prefs_common.chk_on_startup);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_openinbox),
		prefs_common.open_inbox_on_inc);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_scan_after_inc),
		prefs_common.scan_all_after_inc);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_newmail_auto),
		prefs_common.newmail_notify_auto);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_newmail_manu),
		prefs_common.newmail_notify_manu);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbtn_autochk),
		prefs_common.autochk_newmail);

	gtk_entry_set_text(GTK_ENTRY(entry_incext), 
		prefs_common.extinc_cmd);
	gtk_entry_set_text(GTK_ENTRY(entry_newmail_notify_cmd), 
		prefs_common.newmail_notify_cmd);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbtn_autochk), 
		prefs_common.autochk_itv);
	gtk_option_menu_set_history(GTK_OPTION_MENU(optmenu_recvdialog),
		prefs_common.recv_dialog_mode);

	prefs_receive->window = GTK_WIDGET(window);
	prefs_receive->checkbtn_incext = checkbtn_incext;
	prefs_receive->entry_incext = entry_incext;
	prefs_receive->checkbtn_autochk = checkbtn_autochk;
	prefs_receive->spinbtn_autochk = spinbtn_autochk;
	prefs_receive->checkbtn_chkonstartup = checkbtn_chkonstartup;
	prefs_receive->checkbtn_openinbox = checkbtn_openinbox;
	prefs_receive->checkbtn_scan_after_inc = checkbtn_scan_after_inc;
	prefs_receive->checkbtn_newmail_auto = checkbtn_newmail_auto;
	prefs_receive->checkbtn_newmail_manu = checkbtn_newmail_manu;
	prefs_receive->entry_newmail_notify_cmd = entry_newmail_notify_cmd;
	prefs_receive->optmenu_recvdialog = optmenu_recvdialog;
	prefs_receive->checkbtn_no_recv_err_panel = checkbtn_no_recv_err_panel;
	prefs_receive->checkbtn_close_recv_dialog = checkbtn_close_recv_dialog;
	prefs_receive->hbox_newmail_notify = hbox_newmail_notify;
	prefs_receive->page.widget = vbox1;

	g_signal_connect(G_OBJECT(checkbtn_newmail_auto), "toggled",
			 G_CALLBACK(prefs_common_recv_dialog_newmail_notify_toggle_cb),
			 NULL);
	g_signal_connect(G_OBJECT(checkbtn_newmail_manu), "toggled",
			 G_CALLBACK(prefs_common_recv_dialog_newmail_notify_toggle_cb),
			 NULL);

}

void prefs_receive_save(PrefsPage *_page)
{
	ReceivePage *page = (ReceivePage *) _page;
	gchar *tmp;
	GtkWidget *menu;
	GtkWidget *menuitem;

	prefs_common.use_extinc = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_incext));
	prefs_common.no_recv_err_panel = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_no_recv_err_panel));
	prefs_common.close_recv_dialog = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_close_recv_dialog));
	prefs_common.chk_on_startup = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_chkonstartup));
	prefs_common.open_inbox_on_inc = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_openinbox));
	prefs_common.scan_all_after_inc = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_scan_after_inc));
	prefs_common.newmail_notify_auto = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_newmail_auto));
	prefs_common.newmail_notify_manu = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_newmail_manu));
	prefs_common.autochk_newmail = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->checkbtn_autochk));
	prefs_common.autochk_itv = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(page->spinbtn_autochk));
	
	tmp = gtk_editable_get_chars(GTK_EDITABLE(page->entry_incext), 0, -1);
	g_free(prefs_common.extinc_cmd);
	prefs_common.extinc_cmd = tmp;
	
	tmp = gtk_editable_get_chars(GTK_EDITABLE(page->entry_newmail_notify_cmd), 0, -1);
	g_free(prefs_common.newmail_notify_cmd);
	prefs_common.newmail_notify_cmd = tmp;
	
	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(page->optmenu_recvdialog));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	prefs_common.recv_dialog_mode = GPOINTER_TO_INT
		(g_object_get_data(G_OBJECT(menuitem), MENU_VAL_ID));
	inc_autocheck_timer_remove();
	inc_autocheck_timer_set();

}

static void prefs_receive_destroy_widget(PrefsPage *_page)
{
}

void prefs_receive_init(void)
{
	ReceivePage *page;
	static gchar *path[3];

	path[0] = _("Mail Handling");
	path[1] = _("Receive");
	path[2] = NULL;

	page = g_new0(ReceivePage, 1);
	page->page.path = path;
	page->page.create_widget = prefs_receive_create_widget;
	page->page.destroy_widget = prefs_receive_destroy_widget;
	page->page.save_page = prefs_receive_save;
	page->page.weight = 200.0;
	prefs_gtk_register_page((PrefsPage *) page);
	prefs_receive = page;
}

void prefs_receive_done(void)
{
	prefs_gtk_unregister_page((PrefsPage *) prefs_receive);
	g_free(prefs_receive);
}

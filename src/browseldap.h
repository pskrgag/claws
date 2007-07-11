/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2003-2007 Match Grun and the Claws Mail team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

/*
 * Browse LDAP entry.
 */

#ifndef __BROWSE_LDAP_H__
#define __BROWSE_LDAP_H__

#ifdef USE_LDAP

#include "addrindex.h"

gboolean browseldap_entry( AddressDataSource *ds, const gchar *dn );

#endif /* USE_LDAP */

#endif /* __BROWSE_LDAP_H__ */

/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2011  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef PLUGINS_LYRE_LYRE_H
#define PLUGINS_LYRE_LYRE_H

#include <QObject>
#include <QDebug>
#include <QIcon>

#include <interfaces/iinfo.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/entitytesthandleresult.h>

#include "lyrewidget.h"

namespace LeechCraft
{
namespace Lyre
{
	class Plugin : public QObject
			, public IInfo
			, public IHaveTabs
			, public IEntityHandler
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IHaveTabs IEntityHandler)
		ICoreProxy_ptr Proxy_;
		TabClasses_t TabClasses_;
		LyreWidget Lmd_;

	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		void Remove ();
		void TabOpenRequested (const QByteArray&);
		void Handle (LeechCraft::Entity);
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;
		TabClassInfo MainTab_;
		TabClasses_t GetTabClasses () const;
		TabClassInfo GetTabClassInfo () const;
		QObject* ParentMultiTabs ();
		QToolBar* GetToolBar () const;
		EntityTestHandleResult CouldHandle (const LeechCraft::Entity&) const;

	signals:
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);
	};
}
}

#endif


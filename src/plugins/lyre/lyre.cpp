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

#include "lyre.h"

namespace LeechCraft
{
namespace Lyre
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		MainTab_.TabClass_ = "Lyre";
		MainTab_.VisibleName_ = tr ("Lyre");
		MainTab_.Description_ = tr ("Lyre - MPD client module plugin");
		MainTab_.Icon_ = QIcon ();
		MainTab_.Priority_ = 0;
		MainTab_.Features_ = TFOpenableByRequest;
		TabClasses_ << MainTab_;
	}
	void Plugin::SecondInit ()
	{
	}
	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.lyre";
	}
	void Plugin::Release ()
	{
	}
	QString Plugin::GetName () const
	{
		return "Lyre";
	}
	QString Plugin::GetInfo () const
	{
		return tr ("MPD client module");
	}
	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}
	TabClasses_t Plugin::GetTabClasses () const
	{
		return TabClasses_;
	}
	TabClassInfo Plugin::GetTabClassInfo () const
	{
		return MainTab_;
	}
	QObject* Plugin::ParentMultiTabs ()
	{
		return 0;
	}
	void Plugin::Remove ()
	{
		emit removeTab (qobject_cast<QWidget *> (&Lmd_));
	}
	QToolBar* Plugin::GetToolBar () const
	{
		return 0;
	}
	void Plugin::TabOpenRequested (const QByteArray &tabID)
	{
		emit addNewTab ("Lyre", qobject_cast<QWidget *> (&Lmd_));
	}
	EntityTestHandleResult Plugin::CouldHandle (const LeechCraft::Entity&) const
	{
		return EntityTestHandleResult (EntityTestHandleResult::PIdeal);
	}
	void Plugin::Handle (LeechCraft::Entity)
	{
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_lyre, LeechCraft::Lyre::Plugin);


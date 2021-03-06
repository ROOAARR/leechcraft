/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#ifndef PLUGINS_AGGREGATOR_CHANNELSMODEL_H
#define PLUGINS_AGGREGATOR_CHANNELSMODEL_H
#include <QAbstractItemModel>
#include "channel.h"

class QToolBar;
class QMenu;

namespace LeechCraft
{
namespace Aggregator
{
	class ChannelsModel : public QAbstractItemModel
	{
		Q_OBJECT

		QStringList Headers_;
		typedef QList<ChannelShort> Channels_t;
		Channels_t Channels_;
		QToolBar *Toolbar_;
		QWidget *TabWidget_;
		QMenu *Menu_;
	public:
		enum Columns
		{
			ColumnTitle,
			ColumnUnread,
			ColumnLastBuild
		};
		ChannelsModel (QObject *parent = 0);
		virtual ~ChannelsModel ();

		void SetWidgets (QToolBar*, QWidget*);

		virtual int columnCount (const QModelIndex& = QModelIndex ()) const;
		virtual QVariant data (const QModelIndex&, int = Qt::DisplayRole) const;
		virtual Qt::ItemFlags flags (const QModelIndex&) const;
		virtual QVariant headerData (int, Qt::Orientation, int = Qt::DisplayRole) const;
		virtual QModelIndex index (int, int, const QModelIndex& = QModelIndex()) const;
		virtual QModelIndex parent (const QModelIndex&) const;
		virtual int rowCount (const QModelIndex& = QModelIndex ()) const;

		void AddChannel (const ChannelShort&);
		void Update (const channels_container_t&);
		void UpdateChannelData (const ChannelShort&);
		ChannelShort& GetChannelForIndex (const QModelIndex&);
		void RemoveChannel (const ChannelShort&);

		void Clear ();

		QModelIndex GetUnreadChannelIndex () const;
		int GetUnreadChannelsNumber () const;
		int GetUnreadItemsNumber () const;

		void SetMenu (QMenu*);
	signals:
		void channelDataUpdated (IDType_t id, IDType_t feedId);
	};
}
}

#endif

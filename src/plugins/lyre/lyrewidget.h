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
#ifndef PLUGINS_LYRE_LYREWIDGET_H
#define PLUGINS_LYRE_LYREWIDGET_H

#include <QStandardItem>
#include <QStandardItemModel>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QString>
#include <QTimer>
#include <QDebug>

#include <interfaces/ihavetabs.h>

#include "core.h"
#include "treeview.h"
#include "ui_lyrewidget.h"

namespace LeechCraft
{
namespace Lyre
{

class LyreWidget : public QWidget
			, public ITabWidget
{
	Q_OBJECT
	Q_INTERFACES (ITabWidget)

public:
	LyreWidget (QObject *parent = 0);
    ~LyreWidget();
	LeechCraft::TabClassInfo GetTabClassInfo() const;
	QObject* ParentMultiTabs();
	QToolBar* GetToolBar() const;
	void Remove();

private:
	Ui::LyreWidget ui;
	TabClassInfo mainTab;
	LyreCore Core;
	QStandardItem RootItem;
	QStandardItemModel LibraryModel;
	QStandardItemModel PlaylistModel;
	QModelIndex CurrentSongPos;
	QTimer timer;
	void fillTree (LyreMpdEntity entity, QStandardItem *item);
	void fillList ();
	void fillAll ();
	void playReq (unsigned int pos);
	void remSong (unsigned int pos);

signals:
	void removeTab (QWidget*);

private slots:
	void playReq ();
	void playReq (QModelIndex);
	void pauseReq ();
	void stopReq ();
	void fwdReq ();
	void bwdReq ();
	void seekReq ();
	void setCurrentSongPos (QModelIndex);
	void clearReq ();
	void setVolumeReq (int);
	void addSongToQueue (QString uri);
	void remSongFromQueue ();
	void ConnectToMpd ();
	void ReconnectToMpd ();
	void DisconnectFromMpd ();
	void MpdRescanReq ();
	void MpdUpdateReq ();
	void UpdateStatus ();
};

}
}
#endif // LYREWIDGET_H

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
#include "lyrewidget.h"

namespace LeechCraft
{
namespace Lyre
{
LyreWidget::LyreWidget ( QObject* parent ) : QWidget()
{
	qDebug () << Q_FUNC_INFO;
	
	ui.setupUi(this);
	
	connect (ui.viewPlaylist,     SIGNAL (clicked (QModelIndex)),       this, SLOT (setCurrentSongPos (QModelIndex)));
	connect (ui.viewPlaylist,     SIGNAL (doubleClicked (QModelIndex)), this, SLOT (playReq (QModelIndex)));
	connect (ui.pbtnPlay,         SIGNAL (clicked ()),                  this, SLOT (playReq ()));
	connect (ui.pbtnPause,        SIGNAL (clicked ()),                  this, SLOT (pauseReq ()));
	connect (ui.pbtnStop,         SIGNAL (clicked ()),                  this, SLOT (stopReq ()));
	connect (ui.pbtnFwd,          SIGNAL (clicked ()),                  this, SLOT (fwdReq ()));
	connect (ui.pbtnBwd,          SIGNAL (clicked ()),                  this, SLOT (bwdReq ()));
	connect (ui.pbtnDelete,       SIGNAL (clicked()),                   this, SLOT (remSongFromQueue ()));
	connect (ui.pbtnClear,        SIGNAL (clicked ()),                  this, SLOT (clearReq ()));
	connect (ui.volume,           SIGNAL (valueChanged (int)),          this, SLOT (setVolumeReq (int)));
	connect (ui.viewPlaylist,     SIGNAL (songAdded (QString)),         this, SLOT (addSongToQueue (QString)));
	connect (ui.actionConnect,    SIGNAL (triggered()),                 this, SLOT (ConnectToMpd ()));
	connect (ui.actionDisconnect, SIGNAL (triggered()),                 this, SLOT (DisconnectFromMpd ()));
	connect (ui.actionRescan,     SIGNAL (triggered ()),                this, SLOT (MpdRescanReq ()));
	connect (ui.actionUpdate,     SIGNAL (triggered ()),                this, SLOT (MpdUpdateReq ()));
	connect (&timer,              SIGNAL (timeout ()),                  this, SLOT (UpdateStatus ()));
	connect (ui.TrackProgress,    SIGNAL (sliderReleased ()),           this, SLOT (seekReq ()));

	mainTab.TabClass_ = "Lyre";
	mainTab.VisibleName_ = tr ("Lyre");
	mainTab.Description_ = tr ("Lyre - MPD client module plugin");
	mainTab.Icon_ = QIcon ();
	mainTab.Priority_ = 0;
	mainTab.Features_ = TFOpenableByRequest;
	
	ui.actionDisconnect->setEnabled (false);
	//Disable player controls until connect
	ui.pbtnPlay->setEnabled (false);
	ui.pbtnStop->setEnabled (false);
	ui.pbtnPause->setEnabled (false);
	ui.pbtnFwd->setEnabled (false);
	ui.pbtnBwd->setEnabled (false);
	ui.pbtnDelete->setEnabled (false);
	ui.pbtnClear->setEnabled (false);
	ui.volume->setEnabled (false);
	
	ui.lblTrackProgress->setText ("--:--");
	ui.lblTrackName->setText ("");
	ui.TrackProgress->setTracking (false);

	//TODO implement rescan and update///
	ui.actionRescan->setEnabled (false);
	ui.actionUpdate->setEnabled (false);
	/////////////////////////////////////
}

LyreWidget::~LyreWidget()
{
}

void LyreWidget::fillTree (LyreMpdEntity entity, QStandardItem *item)
{
	qDebug () << Q_FUNC_INFO << "ENTER";
	//hash.insert (item, entity);
	for (int i = 0; i < entity.entities.count (); i++)
	{
		QStandardItem *child = new QStandardItem (entity.entities[i].getName ());
		child->setData (entity.entities[i].getURI ());
		item->appendRow (child);
		fillTree (entity.entities[i], child);
	}
	qDebug () << Q_FUNC_INFO << "EXIT";
}

void LyreWidget::fillList ()
{
	qDebug () << Q_FUNC_INFO << "ENTER";
	PlaylistModel.clear();
	QList <LyreMpdTrack> queue = Core.getQueue ();
	for (int i = 0; i < queue.count (); i++)
	{
		QList<QStandardItem*> items;
		LyreMpdTrack song = queue[i];
		QString name = song.getName ();
		unsigned int id = song.getPos ();
		items.append (new QStandardItem (QString::number (i+1)));
		items.last ()->setData (QVariant (id));
		items.append (new QStandardItem (name));
		items.last ()->setData (QVariant (id));
		PlaylistModel.appendRow (items);
		qDebug () << items[0]->data ();
		//ui->viewPlaylist->resizeColumnsToContents();
	}
	qDebug () << Q_FUNC_INFO << "EXIT";
}

void LyreWidget::fillAll()
{
	qDebug () << Q_FUNC_INFO << "ENTER";
//	LibraryModel.clear();
	RootItem.removeRows (0, RootItem.rowCount ());
	RootItem.setText("/");
	fillTree (Core.getMpdContent (), &RootItem);
	LibraryModel.appendRow (&RootItem);
	ui.viewLibrary->setModel (&LibraryModel);
//	PlaylistModel->clear();
	fillList ();
	ui.viewPlaylist->setModel (&PlaylistModel);
	qDebug () << Q_FUNC_INFO << "EXIT";
}

void LyreWidget::playReq(unsigned int pos)
{
	qDebug () << Q_FUNC_INFO;
	Core.play (pos);
	//timer.start (1000);
}

void LyreWidget::playReq()
{
	qDebug () << Q_FUNC_INFO << "ENTER";
	
	unsigned int pos = 0;
	
	if (CurrentSongPos.isValid ())
	{
		qDebug () << "CurrentSongPos is valid";
		QStandardItem* item = PlaylistModel.itemFromIndex (CurrentSongPos);
		pos = item->data ().toUInt ();
	}
	
	playReq(pos);
	
	qDebug () << Q_FUNC_INFO << "EXIT";
}

void LyreWidget::pauseReq()
{
	Core.pause ();
}

void LyreWidget::stopReq()
{
	//timer.stop ();
	ui.lblTrackProgress->setText ("--:--");
	ui.lblTrackName->setText ("");
	Core.stop ();
}

void LyreWidget::fwdReq ()
{
	Core.forward ();
}

void LyreWidget::bwdReq ()
{
	Core.backward ();
}

void LyreWidget::setCurrentSongPos (QModelIndex arg)
{
	CurrentSongPos = arg;
}

void LyreWidget::playReq (QModelIndex arg)
{
	setCurrentSongPos (arg);
	playReq ();
}


void LyreWidget::clearReq ()
{
	//timer.stop ();
	ui.lblTrackProgress->setText ("--:--");
	ui.lblTrackName->setText ("");
	Core.clear ();
	PlaylistModel.clear ();
}

void LyreWidget::setVolumeReq (int arg)
{
	qDebug () << Q_FUNC_INFO;
	Core.setVolume (arg);
}

LeechCraft::TabClassInfo LyreWidget::GetTabClassInfo() const
{
	return mainTab;
}

QObject* LyreWidget::ParentMultiTabs()
{
	return 0;
}

void LyreWidget::Remove()
{
	qDebug () << Q_FUNC_INFO;
	emit removeTab (qobject_cast<QWidget *> (this));
}

QToolBar* LyreWidget::GetToolBar() const
{
	return 0;
}

void LyreWidget::addSongToQueue (QString uri)
{
	qDebug () << Q_FUNC_INFO;
	Core.addSongToQueue (uri);
	fillList ();
}

void LyreWidget::remSongFromQueue ()
{
	int pos;
	if (CurrentSongPos.isValid ())
	{
		QStandardItem* item = PlaylistModel.itemFromIndex (CurrentSongPos);
		pos = item->data ().toInt ();
	}
	else
	{
		pos = 0;
	}
	Core.remSongFromQueue (pos);
	fillList();
}

void LyreWidget::ConnectToMpd ()
{
	if (Core.connect ("localhost", 6600, 3000)) //params should be got from options or?
	{
		RootItem.setText("/");
		fillAll();
		ui.actionConnect->setEnabled (false);
		ui.actionDisconnect->setEnabled (true);
		timer.start (1000);
		ui.pbtnPlay->setEnabled (true);
		ui.pbtnStop->setEnabled (true);
		ui.pbtnPause->setEnabled (true);
		ui.pbtnFwd->setEnabled (true);
		ui.pbtnBwd->setEnabled (true);
		ui.pbtnDelete->setEnabled (true);
		ui.pbtnClear->setEnabled (true);
		ui.volume->setEnabled (true);
	} else {
		qDebug () << "Can't connect to MPD server";
	}
}

void LyreWidget::ReconnectToMpd ()
{
	DisconnectFromMpd ();
	ConnectToMpd ();
}

void LyreWidget::DisconnectFromMpd ()
{
	timer.stop ();
	Core.disconnect ();
	ui.actionConnect->setEnabled (true);
	ui.actionDisconnect->setEnabled (false);
	ui.pbtnPlay->setEnabled (false);
	ui.pbtnStop->setEnabled (false);
	ui.pbtnPause->setEnabled (false);
	ui.pbtnFwd->setEnabled (false);
	ui.pbtnBwd->setEnabled (false);
	ui.pbtnDelete->setEnabled (false);
	ui.pbtnClear->setEnabled (false);
	ui.volume->setEnabled (false);
	ui.lblTrackProgress->setText ("--:--");
	ui.lblTrackName->setText ("");
}

void LyreWidget::MpdRescanReq ()
{
	qDebug () << Q_FUNC_INFO;
//	LibraryModel.clear ();
//	RootItem = QStandardItem::QStandardItem ("/");
	Core.rescanLibrary ("/");
//	fillTree (Core.getMpdContent (), &RootItem);
	fillAll ();
}

void LyreWidget::MpdUpdateReq ()
{
	qDebug () << Q_FUNC_INFO;
//	LibraryModel.clear ();
//	RootItem = QString ("/");
	Core.updateLibrary ("/");
//	fillTree (Core.getMpdContent (), &RootItem);
	fillAll ();
}

void LyreWidget::UpdateStatus ()
{
	qDebug () << Q_FUNC_INFO;
	CurrentStatus CurrentStatus = Core.getCurrentStatus ();
	if (CurrentStatus.SongElapsedTime != -1)
	{
		if (CurrentStatus.SongElapsedTime < 3600)
		{
			ui.lblTrackProgress->setText (ui.lblTrackProgress->text ().sprintf ("%02d:%02d", CurrentStatus.SongElapsedTime / 60, 
																							 CurrentStatus.SongElapsedTime % 60));
		} 
		else 
		{
			ui.lblTrackProgress->setText (ui.lblTrackProgress->text ().sprintf ("%d:%02d:%02d", CurrentStatus.SongElapsedTime / 3600, 
																								CurrentStatus.SongElapsedTime  % 60 / 60, 
																								CurrentStatus.SongElapsedTime  % 60));	
		}
		
		if (CurrentStatus.SongDuration < 3600)
		{
			ui.lblTrackDuration->setText (ui.lblTrackDuration->text ().sprintf ("%02d:%02d", CurrentStatus.SongDuration / 60,
																							 CurrentStatus.SongDuration % 60));
		}
		else
		{
			ui.lblTrackDuration->setText (ui.lblTrackDuration->text ().sprintf ("%d:%02d:%02d", CurrentStatus.SongDuration / 3600,
																								CurrentStatus.SongDuration % 60 / 60,
																								CurrentStatus.SongDuration % 60));
		}
		
		ui.lblTrackName->setText (CurrentStatus.SongName);
		ui.TrackProgress->setMaximum (CurrentStatus.SongDuration);
		if (!ui.TrackProgress->isSliderDown ())
		{
			ui.TrackProgress->setValue (CurrentStatus.SongElapsedTime);
		}
		else
		{
			qDebug () << "slider is pressed";
		}
	}
	else
	{
		ui.lblTrackProgress->setText ("--:--");
		ui.lblTrackDuration->setText ("--:--");
		ui.lblTrackName->setText ("");
	}

	ui.volume->setValue (CurrentStatus.Volume);
}

void LyreWidget::seekReq ()
{
	qDebug () << Q_FUNC_INFO;
	Core.seekReq (ui.TrackProgress->sliderPosition ());
}


}
}

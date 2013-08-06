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
#ifndef PLUGINS_LYRE_CORE_H
#define PLUGINS_LYRE_CORE_H

#include <QObject>
#include <QVector>
#include <QDebug>

#include "client.h"

class LyreCore : public QObject
{
	Q_OBJECT
public:
	bool connect (QString host, int port, int timeout);
	void reconnect ();
	void disconnect ();
	void play (unsigned int id);
	void pause ();
	void stop ();
	void forward ();
	void backward ();
	void clear ();
	void setVolume (int value);
	void addSongToQueue (QString uri);
	void remSongFromQueue (unsigned int pos);
	void rescanLibrary (QString path);
	void updateLibrary (QString path);
	LyreMpdEntity getMpdContent ();
	QList <LyreMpdTrack> getQueue ();
	CurrentStatus getCurrentStatus ();
	void seekReq (unsigned int pos);

private:
	LyreMpdClient Client_;
	LyreMpdEntity RootEntity_;
	QList <LyreMpdTrack> Queue_;

};

#endif

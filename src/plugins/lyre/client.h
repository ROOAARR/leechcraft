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
#ifndef PLUGINS_LYRE_CLIENT_H
#define PLUGINS_LYRE_CLIENT_H

#include <QObject>
#include <QDebug>

#include "entity.h"
#include "track.h"
#include "mpd/client.h"

struct CurrentStatus
{
	QString SongName;
	int SongElapsedTime;
	int SongDuration;
	int Volume;
};

class LyreMpdClient : public QObject
{
	Q_OBJECT
public:
	LyreMpdClient ();
	~LyreMpdClient ();
	bool connect (QString server, int port, int timeout);
	void disconnect ();
	void getLibrary (LyreMpdEntity *entity);
	QList<LyreMpdTrack> getQueue ();
	void clearQueue ();
	void addSongToQueue (QString uri);
	void remSongFromQueue (int id);
	void rescanLibrary (QString path);
	void updateLibrary (QString path);
	void play (unsigned int pos);
	void pause ();
	void stop ();
	void playNext ();
	void playPrevious ();
	CurrentStatus getCurrentStatus ();
	void setVolume (int value);
	void seekReq (unsigned int);

private:
	struct mpd_connection *connection;
	void scanLibrary (LyreMpdEntity *entity);
	
signals:
	
public slots:

};

#endif // CLIENT_H

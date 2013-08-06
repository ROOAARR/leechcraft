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
#include <QDebug>
#include "client.h"

LyreMpdClient::LyreMpdClient ()
{
	connection = 0;
}

LyreMpdClient::~LyreMpdClient ()
{
	mpd_connection_free (connection);
}

bool LyreMpdClient::connect (QString server, int port, int timeout)
{
	connection = mpd_connection_new (server.toAscii().data(), port, timeout);
	mpd_error err = mpd_connection_get_error (connection);
	switch (err)
	{
		case MPD_ERROR_SUCCESS:
		{
			return true;
		}
		case MPD_ERROR_OOM:
		{
			//out of memory
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_ARGUMENT:
		{
			//function was called with an unrecognized or invalid argument
			qDebug () << mpd_connection_get_error_message (connection); 
			return false;
		}
		case MPD_ERROR_STATE:
		{
			//function was called which is not available in the current state of libmpdclient
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_TIMEOUT:
		{
			//timeout trying to talk to mpd
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_SYSTEM:
		{
			//system error
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_RESOLVER:
		{
			//unknown host"
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_MALFORMED:
		{
			//malformed response received from MPD
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_CLOSED:
		{
			//connection closed by mpd
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
		case MPD_ERROR_SERVER:
		{
			 //The server has returned an error code, which can be queried with mpd_connection_get_server_error()
			qDebug () << mpd_connection_get_error_message (connection);
			return false;
		}
	}
}

void LyreMpdClient::disconnect ()
{
	mpd_connection_free (connection);
	connection = 0;
}

void LyreMpdClient::getLibrary (LyreMpdEntity *entity)
{
	qDebug() << Q_FUNC_INFO << "ENTER";
	if (connection == 0)
	{
		qDebug () << "Connection to MPD server doesn't exist";
	}
	else
	{
		scanLibrary (entity);	
	}
	qDebug () << Q_FUNC_INFO << "EXIT";
}

void LyreMpdClient::scanLibrary (LyreMpdEntity *entity)
{
	qDebug () << Q_FUNC_INFO << "ENTER";
	qDebug () << entity->getURI ().toStdString ().data (); 

	if (mpd_send_list_meta (connection, entity->getURI ().toStdString ().data ()))
	{
		mpd_entity *tmpEntity = mpd_recv_entity (connection);
		while (tmpEntity)
		{
			entity->entities.append (LyreMpdEntity (tmpEntity));
			mpd_entity_free (tmpEntity);
			tmpEntity = mpd_recv_entity (connection);
		}
		for (int i = 0; i < entity->entities.count(); i++)
		{
			if (entity->entities[i].getType () == LyreMpdEntity::entityType::DIRECTORY)
			{
				scanLibrary (&entity->entities[i]);
			}
		}
	} 
}

QList<LyreMpdTrack> LyreMpdClient::getQueue ()
{
	QList<LyreMpdTrack> queue;
	if (connection && mpd_send_list_queue_meta (connection))
	{
		mpd_song *song = mpd_recv_song (connection);
		while (song)
		{
			queue.append (LyreMpdTrack (song));
			mpd_song_free (song);
			song = mpd_recv_song (connection);
		}
	}
	qDebug () << queue.count ();
	return queue;
}

void LyreMpdClient::clearQueue ()
{
	if (connection)
	{
		mpd_run_clear (connection);
	}
}

void LyreMpdClient::addSongToQueue (QString uri)
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_add (connection, uri.toAscii());
}

void LyreMpdClient::remSongFromQueue (int id)
{
	mpd_run_delete (connection, id);
}

void LyreMpdClient::rescanLibrary (QString path)
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_rescan (connection, path.toAscii ());
}

void LyreMpdClient::updateLibrary (QString path)
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_update (connection, path.toAscii ());
}

void LyreMpdClient::play (unsigned int pos)
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_play_pos (connection, pos);
}

void LyreMpdClient::pause ()
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_toggle_pause (connection);
}

void LyreMpdClient::playNext ()
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_next (connection);
}

void LyreMpdClient::playPrevious ()
{
	qDebug () << Q_FUNC_INFO;
	qDebug () << mpd_run_previous (connection);
}

CurrentStatus LyreMpdClient::getCurrentStatus ()
{
	struct mpd_status *status;
	CurrentStatus curStatus;
	if (mpd_send_status (connection))
	{
		status = mpd_recv_status (connection);
		if (status)
		{
			if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
                mpd_status_get_state(status) == MPD_STATE_PAUSE)
			{
				curStatus.SongName = getQueue ()[mpd_status_get_song_pos (status)].getName ();
				curStatus.SongElapsedTime = mpd_status_get_elapsed_time (status);
				curStatus.SongDuration = mpd_status_get_total_time (status);
			}
			else
			{
				curStatus.SongName = "";
				curStatus.SongElapsedTime = -1;
				curStatus.SongDuration = -1;
			}
			curStatus.Volume = mpd_status_get_volume (status);
		}
	}
	return curStatus;
}

void LyreMpdClient::setVolume (int volume)
{
	mpd_run_set_volume (connection, volume);
}

void LyreMpdClient::stop ()
{
	mpd_run_stop (connection);
}

void LyreMpdClient::seekReq (unsigned int pos)
{
	struct mpd_status *status;
	CurrentStatus curStatus;
	if (mpd_send_status (connection))
	{
		status = mpd_recv_status (connection);
		if (status)
		{
			if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
                mpd_status_get_state(status) == MPD_STATE_PAUSE)
			{
				mpd_run_seek_pos (connection, mpd_status_get_song_pos (status), pos);
			}
		}
	}
}


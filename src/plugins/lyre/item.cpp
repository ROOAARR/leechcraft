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
#include "item.h"

#include <QVector>

MpdEntityItem::MpdEntityItem()
{
	qDebug () << Q_FUNC_INFO;
}

QVector<MpdEntityItem*> MpdEntityItem::getEntities ()
{
	qDebug () << Q_FUNC_INFO;
	return Entities;
}

void MpdEntityItem::setEntity (mpd_entity* arg)
{
	qDebug () << Q_FUNC_INFO;
	if (arg)
	{
		entity = arg;
		switch (mpd_entity_get_type(entity))
		{
			case MPD_ENTITY_TYPE_DIRECTORY:
			{
//				this->setText (PathToName (mpd_directory_get_path (mpd_entity_get_directory (entity))));
				break;
			}
			case MPD_ENTITY_TYPE_SONG:
			{
//				this->setText (PathToName (mpd_song_get_uri (mpd_entity_get_song (entity))));
				break;
			}
			case MPD_ENTITY_TYPE_PLAYLIST:
			{
//				this->setText (PathToName (mpd_playlist_get_path (mpd_entity_get_playlist (entity))));
				break;
			}
			default:
			{
//				this->setText("Unknown entity");
				break;
			}
		}
	}
}

QString MpdEntityItem::PathToName (QString arg)
{
	qDebug () << Q_FUNC_INFO;
	int pos = arg.lastIndexOf("/");
	if (pos != -1)
	{
		return arg.right (arg.length () - (pos + 1));
	}
	return arg;
}

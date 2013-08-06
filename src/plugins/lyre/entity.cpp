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

#include "entity.h"

LyreMpdEntity::LyreMpdEntity ()
{
	uri = QString ("/");
}

LyreMpdEntity::LyreMpdEntity ( mpd_entity* tmpEntity)
{
	if (tmpEntity)
	{
		switch (mpd_entity_get_type (tmpEntity))
		{
			case MPD_ENTITY_TYPE_DIRECTORY:
			{
				setType (entityType::DIRECTORY);
				setURI  (QString (mpd_directory_get_path (mpd_entity_get_directory (tmpEntity))));
				setName (pathToName (getURI ()));
				break; 
			}
			case MPD_ENTITY_TYPE_SONG:
			{
				setType (entityType::SONG);
				setURI  (QString (mpd_song_get_uri (mpd_entity_get_song (tmpEntity))));
				setName (pathToName (getURI ()));
				break;
			}
			case MPD_ENTITY_TYPE_PLAYLIST:
			{
				setType (entityType::PLAYLIST);
				setURI  (QString (mpd_playlist_get_path (mpd_entity_get_playlist (tmpEntity))));
				setName (pathToName (getURI ()));
				break;
			}
			case MPD_ENTITY_TYPE_UNKNOWN:
			{
				setType (entityType::UNKNOWN);
				setURI  (QString ());
				setName (QString ());
				break;
			}
			default:
			{
				setType (entityType::UNKNOWN);
				setURI  (QString ());
				setName (QString ());
				break;
			}
		}
	}
}

QString LyreMpdEntity::getName () const
{
	return name;
}

void LyreMpdEntity::setName (const QString& argName)
{
	name = argName;
}

QString LyreMpdEntity::getURI () const
{
	return uri;
}

void LyreMpdEntity::setURI (const QString& argURI)
{
	uri = argURI;
}

LyreMpdEntity::entityType LyreMpdEntity::getType () const
{
	return eType;
}

void LyreMpdEntity::setType (const LyreMpdEntity::entityType& typeOfEntity)
{
	eType = typeOfEntity;
}

int LyreMpdEntity::getId ()
{
	return id;
}

void LyreMpdEntity::clean ()
{
	id = -1;
	uri.clear ();
	name.clear ();
	int i = 0;
	foreach (LyreMpdEntity e, entities)
		e.clean ();
	entities.clear ();
}

QString LyreMpdEntity::pathToName (const QString& arg)
{
	int pos = arg.lastIndexOf ("/");
	if (pos != -1)
	{
		return arg.right (arg.length () - (pos + 1));
	}
	return arg;
}

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
#ifndef PLUGINS_LYRE_ENTITY_H
#define PLUGINS_LYRE_ENTITY_H

#include <QVector>
#include <QString>

#include "mpd/client.h"

class LyreMpdEntity
{
public:
	enum class entityType {UNKNOWN, SONG, PLAYLIST,	DIRECTORY};
	LyreMpdEntity ();
	LyreMpdEntity (mpd_entity*);
	void setName (const QString&);
	void setURI (const QString&);
	void setType (const entityType& typeOfEntity);
	void clean ();
	QString getName () const;
	QString getURI () const;
	entityType getType () const;
	int getId ();
	QVector <LyreMpdEntity> entities;
private:
	QString uri;
	QString name;
	QString pathToName (const QString& path);
	int id;
	entityType eType;
};

#endif // LYREMPDENTITY_H

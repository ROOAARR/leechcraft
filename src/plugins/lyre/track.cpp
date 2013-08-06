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
#include "track.h"

LyreMpdTrack::LyreMpdTrack (mpd_song *song)
{
	pos   = mpd_song_get_pos (song);
	uri  = QString (mpd_song_get_uri (song));
	name = PathToName (uri);
}

QString LyreMpdTrack::getName ()
{
	return name;
}

unsigned int LyreMpdTrack::getPos()
{
	return pos;
}

QString LyreMpdTrack::PathToName (QString arg)
{
	int pos = arg.lastIndexOf ("/");
	if (pos != -1)
	{
		return arg.right (arg.length () - (pos + 1));
	}
	return arg;
}


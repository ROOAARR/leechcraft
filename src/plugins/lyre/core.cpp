/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet Client_.
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

#include "core.h"

bool LyreCore::connect (QString host, int port, int timeout)
{
	return Client_.connect (host, port, timeout);
}

void LyreCore::reconnect()
{
	RootEntity_.clean ();
	RootEntity_ = 0;
	Client_.getLibrary (&RootEntity_);
}

void LyreCore::disconnect ()
{
	RootEntity_.clean ();
	RootEntity_ = 0;
	Client_.disconnect ();
}

void LyreCore::play (unsigned int pos)
{
	qDebug () << Q_FUNC_INFO;
	Client_.play (pos);
}

void LyreCore::pause ()
{
	Client_.pause ();
}

void LyreCore::stop ()
{
	Client_.stop ();
}

void LyreCore::forward ()
{
	Client_.playNext ();
}

void LyreCore::backward ()
{
	Client_.playPrevious ();
}

QList<LyreMpdTrack> LyreCore::getQueue ()
{
	return Client_.getQueue ();
}

void LyreCore::clear ()
{
	Client_.clearQueue();
}

void LyreCore::setVolume (int value)
{
	Client_.setVolume (value);
}

void LyreCore::addSongToQueue (QString uri)
{
	Client_.addSongToQueue (uri);
	Queue_ = Client_.getQueue ();
}

void LyreCore::remSongFromQueue (unsigned int pos)
{
	Client_.remSongFromQueue (pos);
	Queue_ = Client_.getQueue ();
}

void LyreCore::updateLibrary (QString path)
{
	Client_.updateLibrary (path);
}

void LyreCore::rescanLibrary (QString path)
{
	Client_.rescanLibrary (path);
}

LyreMpdEntity LyreCore::getMpdContent ()
{
	Client_.getLibrary (&RootEntity_);
	return RootEntity_;
}

CurrentStatus LyreCore::getCurrentStatus ()
{
	return Client_.getCurrentStatus ();
}

void LyreCore::seekReq (unsigned int pos)
{
	Client_.seekReq (pos);
}


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
#include "playlist.h"

LyrePlayList::LyrePlayList(QWidget *parent) :
    QTableView(parent)
{
	qDebug () << Q_FUNC_INFO;
	this->setAcceptDrops (true);
}

void LyrePlayList::dragEnterEvent (QDragEnterEvent * event)
{
	event->accept ();
/*	if (event->mimeData()->hasFormat("text/plain"))
	{
		qDebug () << Q_FUNC_INFO;
		event->acceptProposedAction ();
	}*/
}

void LyrePlayList::dragMoveEvent (QDragMoveEvent * event)
{
	event->accept ();
}

void LyrePlayList::dropEvent (QDropEvent * event)
{
	qDebug () << Q_FUNC_INFO;
	emit songAdded (event->mimeData()->text());
}

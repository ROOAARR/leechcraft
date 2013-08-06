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

#include "treeview.h"

LyreTreeView::LyreTreeView(QWidget *parent) :
    QTreeView(parent)
{
	qDebug () << Q_FUNC_INFO;
}

void LyreTreeView::mouseMoveEvent (QMouseEvent *event)
{
	qDebug () << Q_FUNC_INFO;
	if (!(event->buttons() & Qt::LeftButton))
		return;
/*	if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
		return;*/

	QStandardItemModel *model = qobject_cast <QStandardItemModel*> (this->model ());
	QModelIndex idx = indexAt(event->pos());
	if (model && idx.isValid())
	{
		QDrag *drag = new QDrag(this);
		QMimeData *mimeData = new QMimeData;
		QString mimeType = QString ("text/plain");
		qDebug () << idx.data(Qt::UserRole + 1);
		QVariant song_id =  model->itemFromIndex(indexAt(event->pos()))->data(Qt::UserRole + 1);
		QByteArray data = song_id.toByteArray(); //QString ("very useful data").toAscii ();
		mimeData->setData(mimeType, data);
		drag->setMimeData(mimeData);

		//Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
		drag->exec(Qt::CopyAction);
	}
}
/*
void LyreTreeView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		dragStartPosition = event->pos();
}*/

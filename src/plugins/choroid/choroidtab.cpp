/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2011  Georg Rudoy
 * Copyright (C) 2011 ForNeVeR
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

#include "choroidtab.h"
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QDateTime>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <util/util.h>

namespace LeechCraft
{
namespace Choroid
{
	ChoroidTab::ChoroidTab (const TabClassInfo& tc, QObject *parent)
	: TabClass_ (tc)
	, Parent_ (parent)
	, Scene_ (new QGraphicsScene (this))
	, FSModel_ (new QFileSystemModel (this))
	, FilesModel_ (new QStandardItemModel (this))
	{
		Scene_->setBackgroundBrush (palette ().brush (QPalette::Window));
		Ui_.setupUi (this);

		Ui_.View_->setScene (Scene_);
		Ui_.View_->setRenderHints (QPainter::Antialiasing |
				QPainter::TextAntialiasing |
				QPainter::SmoothPixmapTransform |
				QPainter::HighQualityAntialiasing);
		Ui_.View_->setDragMode (QGraphicsView::ScrollHandDrag);
		Ui_.View_->setOptimizationFlag (QGraphicsView::DontSavePainterState);

		Ui_.VertSplitter_->setStretchFactor (1, 1);
		Ui_.VertSplitter_->setStretchFactor (1, 2);
		Ui_.LeftSplitter_->setStretchFactor (1, 1);
		Ui_.LeftSplitter_->setStretchFactor (1, 2);

		FSModel_->setRootPath (QDir::homePath ());
		FSModel_->setFilter (QDir::AllDirs | QDir::Dirs | QDir::NoDotAndDotDot);
		FSModel_->sort (0);
		Ui_.DirTree_->setModel (FSModel_);
		Ui_.DirTree_->setCurrentIndex (FSModel_->index (QDir::homePath ()));

		connect (Ui_.DirTree_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleDirTreeCurrentChanged (QModelIndex)));

		Ui_.FilesList_->setModel (FilesModel_);

		connect (Ui_.FilesList_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleFileChanged (QModelIndex)));
	}

	TabClassInfo ChoroidTab::GetTabClassInfo () const
	{
		return TabClass_;
	}

	QObject* ChoroidTab::ParentMultiTabs ()
	{
		return Parent_;
	}

	void ChoroidTab::Remove ()
	{
		emit removeTab (this);
	}

	QToolBar* ChoroidTab::GetToolBar () const
	{
		return 0;
	}

	void ChoroidTab::AddThumb (const QFileInfo& info)
	{
		const int spacing = 10;
		const int width = 100;
		const int height = 75;

		QGraphicsPixmapItem *thumb = Scene_->addPixmap (QPixmap (QSize (width, height)));

		const QString& str = fontMetrics ().elidedText (info.fileName (), Qt::ElideRight, width);
		QGraphicsTextItem *text = Scene_->addText (str);

		const int all = DirThumbs_.count () * (spacing + width);
		const int col = all % Ui_.View_->width ();
		const int row = all / Ui_.View_->width ();

		const int fullHeight = height + 5 + fontMetrics ().height ();

		thumb->setPos (col * (width + spacing), row * (fullHeight + spacing));
		text->setPos (col * (width + spacing), row * (fullHeight + spacing) + height + 5);

		DirThumbs_ << thumb;
	}

	void ChoroidTab::handleDirTreeCurrentChanged (const QModelIndex& index)
	{
		const QString& path = FSModel_->filePath (index);
		Ui_.PathEdit_->setText (path);

		FilesModel_->clear ();
		DirThumbs_.clear ();
		Scene_->clear ();

		QStringList headers;
		headers << tr ("Name")
			<< tr ("Size")
			<< tr ("Last modified");
		FilesModel_->setHorizontalHeaderLabels (headers);

		QStringList nf;
		nf << "*.jpg"
			<< "*.png"
			<< "*.svg"
			<< "*.gif";

		Q_FOREACH (const QFileInfo& info,
				QDir (path).entryInfoList (nf, QDir::Files, QDir::Name))
		{
			QList<QStandardItem*> row;
			row << new QStandardItem (info.fileName ());
			row << new QStandardItem (Util::MakePrettySize (info.size ()));
			row << new QStandardItem (info.lastModified ().toString ());

			row.first ()->setData (info.absoluteFilePath (), CRFilePath);

			FilesModel_->appendRow (row);

			AddThumb (info);
		}

		//Scene_->setSceneRect (0, 0, Ui_.View_->width (), DirThumbs_.count () * (100 + 10);
	}

	void ChoroidTab::handleFileChanged (const QModelIndex& index)
	{
		Scene_->clear ();

		if (!index.isValid ())
			return;

		const QString& path = index.sibling (index.row (), 0)
				.data (CRFilePath).toString ();

		QPixmap px (path);
		Scene_->setSceneRect (px.rect ());

		QGraphicsPixmapItem *item = Scene_->addPixmap (px);
		item->ensureVisible ();
		item->setTransformationMode (Qt::SmoothTransformation);
	}
}
}

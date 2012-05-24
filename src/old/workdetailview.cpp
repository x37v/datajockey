/*
 *		Copyright (c) 2008 Alex Norman.  All rights reserved.
 *		http://www.x37v.info/datajockey
 *
 *		This file is part of Data Jockey.
 *		
 *		Data Jockey is free software: you can redistribute it and/or modify it
 *		under the terms of the GNU General Public License as published by the
 *		Free Software Foundation, either version 3 of the License, or (at your
 *		option) any later version.
 *		
 *		Data Jockey is distributed in the hope that it will be useful, but
 *		WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *		Public License for more details.
 *		
 *		You should have received a copy of the GNU General Public License along
 *		with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "workdetailview.hpp"
#include "tagview.hpp"
#include <QLineEdit>
#include <QGridLayout>
#include <QTreeView>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QKeyEvent>
#include <QPushButton>

#include "tagmodel.hpp"
#include "worktagmodelfilter.hpp"

const QString WorkDetailView::cWorkQuery(
		"select audio_works.id id, audio_works.name title, artists.name artist\n"
		"from audio_works\n"
		"\tinner join artists on artists.id = audio_works.artist_id\n"
		"\twhere audio_works.id = ");

WorkDetailView::WorkDetailView(
		TagModel * tagModel,
		const QSqlDatabase & db,
		QWidget * parent) :
	QWidget(parent), 
	mWorkQuery(db)
{
	mDisplayingWork = false;

	mLayout = new QGridLayout(this);
	mArtist = new QLineEdit(tr("artist"), this);
	mTitle = new QLineEdit(tr("title"), this);
	mTagModel = new WorkTagModelFilter(tagModel);
	mTagView = new TagView(mTagModel, this);
	mPreviewButton = new QPushButton("preview", this);
	mPreviewButton->setToolTip(tr("preview selected work on the cue output (toggle)"));

	//don't show count in tagview
	mTagView->setColumnHidden(TagModel::countColumn(),true);
	
	//allow dropping into the tag view
	mTagView->viewport()->setAcceptDrops(true);
	mTagView->setDropIndicatorShown(true);

	//make the preview button toggle
	mPreviewButton->setCheckable(true);

	mArtist->setReadOnly(true);
	mTitle->setReadOnly(true);

	mArtist->setToolTip(tr("artist"));
	mTitle->setToolTip(tr("song title"));

	mLayout->addWidget(mTitle, 0, 0);
	mLayout->addWidget(mArtist, 1, 0);
	mLayout->addWidget(mPreviewButton, 3, 0);
	mLayout->addWidget(mTagView, 0, 1, 4, 1);
	mLayout->setContentsMargins(0,0,0,0);

	mLayout->setColumnStretch(0,0);
	mLayout->setColumnStretch(1,0);
	mLayout->setRowStretch(0,0);
	mLayout->setRowStretch(1,0);
	setLayout(mLayout);

	//connect up internal sigs/slots
	QObject::connect(mPreviewButton,
			SIGNAL(toggled(bool)),
			this,
			SLOT(setPreviewingInternal(bool)));
}

//if a user hits delete and the index is valid then delete that association
void WorkDetailView::keyPressEvent ( QKeyEvent * event ){
	if(event->matches(QKeySequence::Delete) || event->key() == Qt::Key_Backspace){
		if(mTagModel->work() > 0 && mTagView->currentIndex().isValid()){
			mTagModel->sourceTagModel()->removeWorkTagAssociation(mTagModel->work(), mTagView->currentIndex());
			mTagModel->refilter();
		}
	} else 
		QWidget::keyPressEvent(event);
}

void WorkDetailView::setWork(int work_id){
	//build up the queries
	QString workQueryStr(cWorkQuery);
	QString id;
	id.setNum(work_id);
	workQueryStr.append(id);

	//execute and fill in our data
	mWorkQuery.exec(workQueryStr);
	if(mWorkQuery.first()){
		QSqlRecord rec = mWorkQuery.record();
		int titleCol = rec.indexOf("title");
		int artistCol = rec.indexOf("artist");
		mTitle->setText(mWorkQuery.value(titleCol).toString());
		mArtist->setText(mWorkQuery.value(artistCol).toString());
		mTagModel->setWork(work_id);
		mTagView->expandAll();
		mDisplayingWork = true;
		setPreviewing(false);
	} else {
		clear();
	}
}

void WorkDetailView::clear(){
	mArtist->setText(tr("artist"));
	mTitle->setText(tr("title"));
	mTagModel->clear();
	mDisplayingWork = false;
}

void WorkDetailView::expandTags(bool expand){
	if(expand)
		mTagView->expandAll();
	else
		mTagView->collapseAll();
}

void WorkDetailView::setPreviewing(bool down){
	if(down){
		if(mDisplayingWork) {
			mPreviewButton->setText("end preview");
			if(!mPreviewButton->isChecked())
				mPreviewButton->setChecked(true);
		} else
			mPreviewButton->setChecked(false);
	} else {
		mPreviewButton->setText("preview");
		if(mPreviewButton->isChecked())
			mPreviewButton->setChecked(false);
	}
}

void WorkDetailView::setPreviewingInternal(bool down){
	setPreviewing(down);
	emit(previewing(down));
}

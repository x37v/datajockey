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

#include "workpreviewer.hpp"
#include <QSqlRecord>
#include <QVariant>
#include <QTimer>

#define MIN(x,y) ((x) < (y) ? (x) : (y))

WorkPreviewerThread::WorkPreviewerThread(dj::audioIO * audioIO, QObject * parent) : QThread(parent){
	mAudioIO = audioIO;
	mPreviewFrames = audioIO->previewFrames();
	mPreviewBuffer = new jack_default_audio_sample_t[mPreviewFrames * 2];
	mSoundFile = NULL;
	mFillTimer = NULL;
}

WorkPreviewerThread::~WorkPreviewerThread(){
	delete [] mPreviewBuffer;
	if(mFillTimer){
		mFillTimer->stop();
		delete mFillTimer;
	}
	if(mSoundFile)
		delete mSoundFile;
}

void WorkPreviewerThread::playFile(QString file){
	if(mFillTimer == NULL){
		mFillTimer = new QTimer(this);
		//connect us up
		QObject::connect(mFillTimer,
				SIGNAL(timeout()),
				this,
				SLOT(fillBuffer()));
	}

	mFillTimer->stop();
	try {
		if(mSoundFile)
			delete mSoundFile;
		mSoundFile = new SoundFile(file.toStdString());
		if(*mSoundFile){
			//run every millisecond
			mFillTimer->start(1);
		} else {
			delete mSoundFile;
			mSoundFile = NULL;
			emit(playing(false));
		}
	} catch (...) {
		mSoundFile = NULL;
	}
}

void WorkPreviewerThread::stop(){
	if(mFillTimer)
		mFillTimer->stop();
}

void WorkPreviewerThread::run(){
	exec();
}

void WorkPreviewerThread::fillBuffer(){
	if(mSoundFile && mAudioIO->previewFramesFree()){
		//mPreviewBuffer is the length of the audioIO's preview buffer, so we will never overstep
		unsigned int framesRead = mSoundFile->readf(mPreviewBuffer, MIN(mPreviewFrames, mAudioIO->previewFramesFree()));
		if(framesRead > 0){
			mAudioIO->queuePreviewFrames(mPreviewBuffer, framesRead);
		} else {
			//if we haven't read any frames then we stop
			mFillTimer->stop();
			emit(playing(false));
		}
	}
}

QString WorkPreviewer::cFileQueryString(
	"select audio_files.location audio_file\n"
	"from audio_works\n"
	"\tjoin audio_files on audio_files.id = audio_works.audio_file_id\n"
	"where audio_works.id = ");

WorkPreviewer::WorkPreviewer(const QSqlDatabase &db, MixerPanelModel * mixerModel, dj::audioIO * audioIO) :
	mThread(audioIO, this), mFileQuery("", db) 
{
	mWork = -1;
	mMixerPanelModel = mixerModel;
	mThread.start();

	//connect internal signals/slots
	QObject::connect(&mThread,
			SIGNAL(playing(bool)),
			this,
			SLOT(preview(bool)));
	QObject::connect(this,
			SIGNAL(playingFile(QString)),
			&mThread,
			SLOT(playFile(QString)));
	QObject::connect(this,
			SIGNAL(stopping()),
			&mThread,
			SLOT(stop()));
}

void WorkPreviewer::setWork(int work){
	mWork = work;
	emit(previewing(false));
	emit(stopping());
}

void WorkPreviewer::preview(bool p){
	if(p && (mWork > 0)){
		//build up query
		QString fileQueryStr(cFileQueryString);
		QString id;
		id.setNum(mWork);
		fileQueryStr.append(id);
		//execute
		mFileQuery.exec(fileQueryStr);
		QSqlRecord rec = mFileQuery.record();
		int audioFileCol = rec.indexOf("audio_file");

		if(mFileQuery.first()){
			//emit the signal so it is queued up in the thread..
			emit(playingFile(mFileQuery.value(audioFileCol).toString()));
		} else {
			emit(previewing(false));
		}
	} else {
		emit(stopping());
		emit(previewing(false));
	}
}


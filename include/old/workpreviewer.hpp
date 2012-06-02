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

#ifndef WORK_PREVIEWER_HPP
#define WORK_PREVIEWER_HPP

#include <QObject>
#include <QThread>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "soundfile.hpp"
#include "audioio.hpp"

class MixerPanelModel;
class QTimer;

class WorkPreviewerThread : public QThread {
	Q_OBJECT
	public:
		WorkPreviewerThread(dj::audioIO * audioIO, QObject * parent);
		virtual ~WorkPreviewerThread();
	signals:
		void playing(bool playing);
	public slots:
		void playFile(QString file_location);
		void stop();
	protected slots:
		void fillBuffer();
	protected:
		void run();
	private:
		size_t mPreviewFrames;
		jack_default_audio_sample_t * mPreviewBuffer;
		dj::audioIO * mAudioIO;
		QTimer * mFillTimer;
		SoundFile * mSoundFile;
};

class WorkPreviewer : public QObject {
	Q_OBJECT
	public:
		WorkPreviewer(const QSqlDatabase &db, MixerPanelModel * mixerModel, dj::audioIO * audioIO);
	signals:
		void previewing(bool p);
		void playingFile(QString file_location);
		void stopping();
	public slots:
		void setWork(int work);
		void preview(bool p);
	private:
		WorkPreviewerThread mThread;
		int mWork;
		static QString cFileQueryString;
		QSqlQuery mFileQuery;
		MixerPanelModel * mMixerPanelModel;
};

#endif

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

#ifndef WORK_LOADER
#define WORK_LOADER

#include <QThread>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QSqlQuery>
#include "buffer.hpp"
#include <vector>

class MixerPanelModel;
class MixerPanelView;

class BufferLoaderThread : public QThread {
	Q_OBJECT
	public:
		BufferLoaderThread(QObject * parent);
		void start(unsigned int index, int work_id, QString audiobufloc, QString beatbufloc);
	protected:
		void run();
	private:
		unsigned int mIndex;
		int mWorkId;
		QString mAudioBufLoc;
		QString mBeatBufLoc;
	signals:
		void buffersLoaded(unsigned int index, int work_id, 
				DataJockey::AudioBufferPtr audio_buffer, DataJockey::BeatBufferPtr beat_buffer);
		void outOfMemory(unsigned int index, int work_id, QString audioFileLoc, QString beatFileLoc);
		void cannotLoad(unsigned int index, int work_id, QString audioFileLoc, QString beatFileLoc, QString why);
};

class WorkLoader : public QObject {
	Q_OBJECT
	public:
		WorkLoader(const QSqlDatabase & db, MixerPanelModel * mixerModel, MixerPanelView * mixerView);
	public slots:
		void selectWork(int work);
		void mixerLoadWork(unsigned int mixer, int work_id);
	signals:
		void mixerLoaded(unsigned int mixer, DataJockey::AudioBufferPtr audiobuf, 
				DataJockey::BeatBufferPtr beatbuf, bool wait_for_measure = false);
		void mixerLoaded(unsigned int mixer, int work);
	protected slots:
		//this is mapped internally (uses an int because the mapper doesn't do unsigned)
		void mixerLoadWork(int mixer);
		//these are called by the loaderthread
		void setWork(unsigned int mixer_index, 
				int work_id,
				DataJockey::AudioBufferPtr audio_buffer, 
				DataJockey::BeatBufferPtr beat_buffer);
		void outOfMemory(unsigned int index, int work_id, QString audioFileLoc, QString beatFileLoc);
		void cannotLoad(unsigned int index, int work_id, QString audioFileLoc, QString beatFileLoc, QString why);
	private:
		static bool cTypesRegistered;
		static QString cFileQueryString;
		static QString cWorkInfoQueryString;

		//the number of mixers in the model
		unsigned int mNumMixers;
		//the currently selected work
		int mWork;
		std::vector<BufferLoaderThread *> mLoaderThreads;
		MixerPanelView * mMixerPanelView;
		MixerPanelModel * mMixerPanelModel;
		QSqlQuery mFileQuery;
		QSqlQuery mWorkInfoQuery;
};

#endif


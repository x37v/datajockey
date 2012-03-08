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

#include "workloader.hpp"
#include "mixerpanelmodel.hpp"
#include "mixerpanelview.hpp"
#include "mastermodel.hpp"
#include "djmixerchannelview.hpp"
#include "djmixerchannelmodel.hpp"
#include "djmixercontrolview.hpp"
#include "djmixerworkinfoview.hpp"
#include <QString>
#include <QSqlRecord>
#include <QVariant>
#include <QMetaObject>
#include <QErrorMessage>
#include <QSignalMapper>

BufferLoaderThread::BufferLoaderThread(QObject * parent) :
	QThread(parent)
{
}

void BufferLoaderThread::start(unsigned int index, int work_id, QString audiobufloc, QString beatbufloc){
	//set up our data and run
	mIndex = index;
	mWorkId = work_id;
	mAudioBufLoc = audiobufloc;
	mBeatBufLoc = beatbufloc;
	QThread::start();
}

void BufferLoaderThread::run(){
	try {
		DataJockey::BeatBufferPtr beat_buffer = new DataJockey::BeatBuffer(mBeatBufLoc.toStdString());
		DataJockey::AudioBufferPtr audio_file = new DataJockey::AudioBuffer(mAudioBufLoc.toStdString());
		emit(buffersLoaded(mIndex, mWorkId, audio_file, beat_buffer));
	} catch (std::bad_alloc&) {
		emit(outOfMemory(mIndex, mWorkId, mAudioBufLoc, mBeatBufLoc));
	} catch (std::runtime_error e){
		emit(cannotLoad(mIndex, mWorkId, mAudioBufLoc, mBeatBufLoc, QString(e.what())));
	} catch (int e) {
		std::cerr << "An exception occurred. Exception Nr. " << e << std::endl;
	}
}

bool WorkLoader::cTypesRegistered = false;

QString WorkLoader::cFileQueryString(
	"select audio_files.location audio_file, annotation_files.location beat_file\n"
	"from audio_works\n"
	"\tjoin audio_files on audio_files.id = audio_works.audio_file_id\n"
	"\tjoin annotation_files on annotation_files.audio_work_id = audio_works.id\n"
	"where audio_works.id = ");

QString WorkLoader::cWorkInfoQueryString(
		"select audio_works.name title,\n"
		"\tartists.name\n"
		"artist from audio_works"
		"\tinner join artist_audio_works on artist_audio_works.audio_work_id = audio_works.id\n"
		"\tinner join artists on artists.id = artist_audio_works.artist_id\n"
		"where audio_works.id = ");

WorkLoader::WorkLoader(const QSqlDatabase & db, MixerPanelModel * model, MixerPanelView * mixerView) :
	QObject(model), mFileQuery("", db), mWorkInfoQuery("",db)
{
	mWork = -1;
	mMixerPanelView = mixerView;
	mMixerPanelModel = model;
	mNumMixers = model->mixerChannels()->size();
	if(!cTypesRegistered){
		qRegisterMetaType<DataJockey::AudioBufferPtr>("DataJockey::AudioBufferPtr");
		qRegisterMetaType<DataJockey::BeatBufferPtr>("DataJockey::BeatBufferPtr");
		cTypesRegistered = true;
	}

	//we need a signal mapper to map the load() to a mixer id
	QSignalMapper * mixerToIdMapper = new QSignalMapper(this);

	//create our thread pool and connect up our view's "load()" signal to us
	for(unsigned int i = 0; i < mNumMixers; i++){
		BufferLoaderThread * newThread = new BufferLoaderThread(this);
		mLoaderThreads.push_back(newThread);
		//connect it up to us
		QObject::connect(newThread,
				SIGNAL(buffersLoaded(unsigned int, int, DataJockey::AudioBufferPtr, DataJockey::BeatBufferPtr)),
				this,
				SLOT(setWork(unsigned int, int, DataJockey::AudioBufferPtr, DataJockey::BeatBufferPtr)),
				Qt::QueuedConnection);
		QObject::connect(newThread,
				SIGNAL(outOfMemory(unsigned int, int, QString, QString)),
				this,
				SLOT(outOfMemory(unsigned int, int, QString, QString)),
				Qt::QueuedConnection);
		QObject::connect(newThread,
				SIGNAL(cannotLoad(unsigned int, int, QString, QString, QString)),
				this,
				SLOT(cannotLoad(unsigned int, int, QString, QString, QString)),
				Qt::QueuedConnection);
		//set up our mapper [object -> id]
		mixerToIdMapper->setMapping(mixerView->mixerChannels()->at(i)->control(), (int)i);
		//connect the loadClicked signal -> the mapper for this object
		QObject::connect(
				mixerView->mixerChannels()->at(i)->control(),
				SIGNAL(loadClicked()),
				mixerToIdMapper,
				SLOT(map()));
	}
	//connect the mapped signal to our protected mixerLoadWork slot
	QObject::connect(
			mixerToIdMapper,
			SIGNAL(mapped(int)),
			this,
			SLOT(mixerLoadWork(int)));
}

void WorkLoader::selectWork(int work){
	mWork = work;
}

void WorkLoader::mixerLoadWork(unsigned int mixer, int work_id){
	mWork = work_id;
	mixerLoadWork(mixer);
}

void WorkLoader::mixerLoadWork(int mixer){
	if(mWork >= 0 && mixer >= 0 && (unsigned int)mixer < mNumMixers){
		if(mLoaderThreads[mixer]->isRunning()){
			qWarning("Mixer %d is currently loading a file", mixer);
			return;
		}
		//build up query
		QString fileQueryStr(cFileQueryString);
		QString workQueryStr(cWorkInfoQueryString);
		QString id;
		id.setNum(mWork);
		fileQueryStr.append(id);
		workQueryStr.append(id);
		//execute
		mFileQuery.exec(fileQueryStr);
		QSqlRecord rec = mFileQuery.record();
		int audioFileCol = rec.indexOf("audio_file");
		int beatFileCol = rec.indexOf("beat_file");
		//if we can grab it
		if(mFileQuery.first()){
			QString audiobufloc = mFileQuery.value(audioFileCol).toString();
			QString beatbufloc = mFileQuery.value(beatFileCol).toString();

			//if we're syncing to this mixer then change the sync source
			if(mMixerPanelModel->master()->syncSource() == (unsigned int)(mixer + 1))
				mMixerPanelModel->master()->setSyncSource(0);

			//emit a signal to unload the buffers of this mixer
			emit(mixerLoaded(mixer, NULL, NULL));
			//reset the mixer channel
			mMixerPanelModel->mixerChannels()->at(mixer)->reset();
			//use a thread to load the stuff!
			mLoaderThreads[mixer]->start(mixer, mWork, audiobufloc, beatbufloc);

			//indicate that we're loading
			mWorkInfoQuery.exec(workQueryStr);
			if(mWorkInfoQuery.first()){
				rec = mWorkInfoQuery.record();
				int titleCol = rec.indexOf("title");
				int artistCol = rec.indexOf("artist");
				QString loadingArtist("loading: ");
				QString loadingTitle("loading: ");

				loadingArtist.append(mWorkInfoQuery.value(artistCol).toString());
				loadingTitle.append(mWorkInfoQuery.value(titleCol).toString());

				mMixerPanelView->mixerChannels()->at(mixer)->DJMixerWorkInfo()->setArtistText(
						loadingArtist);
				mMixerPanelView->mixerChannels()->at(mixer)->DJMixerWorkInfo()->setTitleText(
						loadingTitle);
				mMixerPanelModel->mixerChannels()->at(mixer)->setWork(-1);
			}
			
		} else {
			//XXX ERROR
		}
	} else {
		//XXX no work selected
	}
}


void WorkLoader::setWork(unsigned int mixer_index, 
		int work,
		DataJockey::AudioBufferPtr audio_buffer, 
		DataJockey::BeatBufferPtr beat_buffer){
	//build up query
	QSqlRecord rec;
	QString workQueryStr(cWorkInfoQueryString);
	QString id;
	id.setNum(work);
	workQueryStr.append(id);

	emit(mixerLoaded(mixer_index, audio_buffer, beat_buffer));
	emit(mixerLoaded(mixer_index, work));
	
	//set the info
	mWorkInfoQuery.exec(workQueryStr);
	if(mWorkInfoQuery.first()){
		rec = mWorkInfoQuery.record();
		int titleCol = rec.indexOf("title");
		int artistCol = rec.indexOf("artist");
		mMixerPanelView->mixerChannels()->at(mixer_index)->DJMixerWorkInfo()->setArtistText(
				mWorkInfoQuery.value(artistCol).toString()
				);
		mMixerPanelView->mixerChannels()->at(mixer_index)->DJMixerWorkInfo()->setTitleText(
				mWorkInfoQuery.value(titleCol).toString()
				);
		mMixerPanelModel->mixerChannels()->at(mixer_index)->setWork(work);
	}
}

void WorkLoader::outOfMemory(unsigned int index, int work_id, QString audioFileLoc, QString beatFileLoc){
	Q_UNUSED (work_id);
	Q_UNUSED (beatFileLoc);
	mMixerPanelView->mixerChannels()->at(index)->DJMixerWorkInfo()->setArtistText(tr("artist"));
	mMixerPanelView->mixerChannels()->at(index)->DJMixerWorkInfo()->setTitleText(tr("title"));
	qWarning("Not enough memory to load audio file:\n %s", audioFileLoc.toStdString().c_str());
}

void WorkLoader::cannotLoad(unsigned int index, int work_id, QString audioFileLoc, QString beatFileLoc, QString why){
	Q_UNUSED (work_id);
	Q_UNUSED (beatFileLoc);
	Q_UNUSED (audioFileLoc);
	mMixerPanelView->mixerChannels()->at(index)->DJMixerWorkInfo()->setArtistText(tr("artist"));
	mMixerPanelView->mixerChannels()->at(index)->DJMixerWorkInfo()->setTitleText(tr("title"));
	qWarning("%s", why.toStdString().c_str());
}


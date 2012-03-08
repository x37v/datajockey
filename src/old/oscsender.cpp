#include "oscsender.hpp"

#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#include <QTimer>
#include "include/mixerpanelmodel.hpp"

/*
	char buffer[OUTPUT_BUFFER_SIZE];

	while(1) {
		msleep(5);
		osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

		p << osc::BeginBundleImmediate
			<< osc::BeginMessage( "/test1" ) 
			<< true << 23 << (float)3.1415 << "hello" << osc::EndMessage
			<< osc::BeginMessage( "/test2" ) 
			<< true << 24 << (float)10.8 << "world" << osc::EndMessage
			<< osc::EndBundle;

		mSocket.Send( p.Data(), p.Size() );
	}
	*/

OscSender::OscSender(std::string audio_addr, int audio_port, MixerPanelModel * mixer) {
	mSocket = new UdpTransmitSocket( IpEndpointName( LOCAL_ADDR, LOCAL_PORT ) );
	mSocketAudio = new UdpTransmitSocket( IpEndpointName( AUDIO_ADDR, AUDIO_PORT ) );
	mMixer = mixer;

	for(unsigned int i = 0; i < 2; i++) {
		mWorks[i] = 0;
		mAudible[i] = false;
		mMaxSample[i] = 0.0;
	}

	mMasterMax = 0.0;

	QObject::connect(
			this, SIGNAL(workIsAubile(int, int)),
			this, SLOT(newWorkAudible(int, int)),
			Qt::QueuedConnection);

	QObject::connect(
			this, SIGNAL(newPeaks(float,float,float)),
			this, SLOT(sendPeaks(float,float,float)),
			Qt::QueuedConnection);
}

void OscSender::run() {

	//poll
	mTimer = new QTimer;
	connect(mTimer, SIGNAL(timeout()), this, SLOT(pollAudible()));
	//every 10 ms
	mTimer->start(10);

	exec();
}

void OscSender::pollAudible() {
	for(unsigned int i = 0; i < 2; i++) {
		if (mMixer->mixerAudible(i)) {
			if(!mAudible[i]) {
				mAudible[i] = true;
				emit(newWorkAudible(i, mWorks[i]));
			}
		} else {
			if(mAudible[i]) {
				mAudible[i] = false;
				emit(newWorkAudible(i, mWorks[i]));
			}
		}
	}
	emit(newPeaks(mMaxSample[0], mMaxSample[1], mMasterMax));
}

void OscSender::newWork(unsigned int mixer, int work_id){
	if (mixer < 2)
		mWorks[mixer] = work_id;
}

void OscSender::newWorkAudible(int mixer, int work_id){

	if (mixer >= 0 && mixer < 2) {
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
		if (mAudible[mixer]) {
			p << osc::BeginBundleImmediate
				<< osc::BeginMessage( "/dj/work/new" ) 
				<< mixer << work_id << osc::EndMessage
				<< osc::EndBundle;
		} else {
			p << osc::BeginBundleImmediate
				<< osc::BeginMessage( "/dj/work/inactive" ) 
				<< mixer << osc::EndMessage
				<< osc::EndBundle;
		}

		mSocket->Send( p.Data(), p.Size() );
	}
}

void OscSender::newTag(int work_id, int tag_id){
	int player = -1;

	if (work_id == mWorks[0])
		player = 0;
	else if (work_id == mWorks[1])
		player = 1;

	if (player >= 0) {
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

		p << osc::BeginBundleImmediate
			<< osc::BeginMessage( "/dj/tag/new" ) 
			<< player << tag_id << osc::EndMessage
			<< osc::EndBundle;

		mSocket->Send( p.Data(), p.Size() );
	}
}

void OscSender::setMixerMax(unsigned int mixer, float max){
	if (mixer < 2) {
		if (mAudible[mixer])
			mMaxSample[mixer] = max;
		else
			mMaxSample[mixer] = 0.0f;
	}
}

void OscSender::setMasterMax(float max){
	mMasterMax = max;
}

void OscSender::sendPeaks(float mixer0, float mixer1, float master){
	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

	p << osc::BeginBundleImmediate
		<< osc::BeginMessage( "/djviz/track/0/audio" ) 
		<< mixer0 << osc::EndMessage
		<< osc::BeginMessage( "/djviz/track/1/audio" ) 
		<< mixer1 << osc::EndMessage
		<< osc::BeginMessage( "/djviz/mix/audio" ) 
		<< master << osc::EndMessage
		<< osc::EndBundle;

	mSocketAudio->Send( p.Data(), p.Size() );
}


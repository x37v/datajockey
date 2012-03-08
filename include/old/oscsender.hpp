#ifndef _OSCSENDER_HPP
#define _OSCSENDER_HPP

#include <QThread>

#define OUTPUT_BUFFER_SIZE 2048

#define LOCAL_ADDR "127.0.0.1"
#define LOCAL_PORT 7000

#define AUDIO_ADDR "10.0.0.1"
#define AUDIO_PORT 10101

class UdpTransmitSocket;
class MixerPanelModel;
class QTimer;

//HACK ASS CLASS
class OscSender : public QThread {
	Q_OBJECT
	public:
		OscSender(std::string audio_addr, int audio_port, MixerPanelModel * mixer);
		void run();
	public slots:
		void newWork(unsigned int mixer, int work_id);
		void newTag(int work_id, int tag_id);
		void setMixerMax(unsigned int mixer, float max);
		void setMasterMax(float max);
	protected slots:
		void sendPeaks(float mixer0, float mixer1, float master);
		void pollAudible();
		void newWorkAudible(int mixer, int work_id);
	signals:
		void workIsAubile(int mixer, int work_id);
		void newPeaks(float mixer0, float mixer1, float master);
	private:
		UdpTransmitSocket * mSocket;
		UdpTransmitSocket * mSocketAudio;
		MixerPanelModel * mMixer;
		QTimer * mTimer;
		bool mAudible[2];
		float mMaxSample[2];
		float mMasterMax;
		int mWorks[2];
};

#endif

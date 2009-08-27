#ifndef DATAJOCKEY_AUDIOIO_HPP
#define DATAJOCKEY_AUDIOIO_HPP

#include "jackaudioio.hpp"
#include "master.hpp"
#include <vector>

namespace DataJockey {
	class AudioIO : public JackCpp::AudioIO {
		public:
			AudioIO();
			Master * master();
			void start();
		protected:
			virtual int audioCallback(
					jack_nframes_t nframes, 
					audioBufVector inBufs,
					audioBufVector outBufs);
			Master * mMaster;
	};
}

#endif

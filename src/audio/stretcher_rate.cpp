#include "stretcher_rate.hpp"

namespace dj {
   namespace audio {
      StretcherRate::~StretcherRate() { }

      void StretcherRate::compute_frame(float * frame, unsigned int new_index, double new_index_subsample, unsigned int last_index, double last_index_subsample) {
         for (unsigned int i = 0; i < 2; i++)
            frame[i] = audio_buffer()->sample(i, new_index, new_index_subsample);
      }
   }
}

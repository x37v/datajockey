#ifndef STRETCHER_RATE_HPP
#define STRETCHER_RATE_HPP

#include "stretcher.hpp"

namespace DataJockey {
   namespace Audio {
      class StretcherRate : public Stretcher {
         public:
            virtual ~StretcherRate();
         protected:
            virtual void compute_frame(float * frame, unsigned int new_index, double new_index_subsample, unsigned int last_index, double last_index_subsample);
      };
   }
}

#endif

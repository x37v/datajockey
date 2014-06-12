#ifndef DJ_ENVELOPE_HPP
#define DJ_ENVELOPE_HPP

#include "defines.hpp"

namespace djaudio {
  typedef double (*envelope_func_t)(double pos);

  class Envelope {
    public:
      Envelope(envelope_func_t func, unsigned int length);

      void length(unsigned int l) {
        mLength = l;
        mLengthMinusOne = l - 1;
      }
      unsigned int length() const { return mLength; }

      void reset();
      void complete();

      bool at_end() const { return mPosition >= mLengthMinusOne; }
      bool at_front() const { return mPosition == 0; }

      void step(bool forward = true);

      double value_at(unsigned int pos) const;
      double reversed_value_at(unsigned int pos) const;

      double value() const;
      double reversed_value() const;
      double value_step(bool forward = true);
    private:
      envelope_func_t mFunc;
      double mLength;
      double mLengthMinusOne;
      double mPosition;
  };

  double half_sin(double p);
  double quarter_sin(double p);
}

#endif

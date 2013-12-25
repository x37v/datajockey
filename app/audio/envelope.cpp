#include "envelope.hpp"
#include <cmath>

using namespace djaudio;

Envelope::Envelope(envelope_func_t func, unsigned int length) :
  mFunc(func),
  mLength(length),
  mLengthMinusOne(length - 1),
  mPosition(0)
{
}

void Envelope::reset() { mPosition = 0; }
void Envelope::complete() { mPosition = mLengthMinusOne; }

void Envelope::step(bool forward) {
  if (forward) {
    if (mPosition < mLengthMinusOne)
      mPosition++;
  } else {
    if (mPosition != 0)
      mPosition--;
  }
}

double Envelope::value_at(unsigned int pos) const { return mFunc(dj::clamp(static_cast<double>(pos) / mLengthMinusOne, 0.0, 1.0)); }
double Envelope::reversed_value_at(unsigned int pos) const { return mFunc(dj::clamp(1.0 - static_cast<double>(pos) / mLengthMinusOne, 0.0, 1.0)); }

double Envelope::value() const { return mFunc(dj::clamp(mPosition / mLengthMinusOne, 0.0, 1.0)); }
double Envelope::reversed_value() const { return mFunc(dj::clamp(1.0 - mPosition / mLengthMinusOne, 0.0, 1.0)); }

double Envelope::value_step(bool forward) {
  double v = value();
  step(forward);
  return v;
}

double djaudio::half_sin(double p) { return sin(p * dj::pi); }
double djaudio::quarter_sin(double p) { return half_sin(p * 0.5); }

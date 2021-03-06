#include "transport.hpp"

using namespace djaudio;

Transport::Transport(){
   mPosition.at_bar(0);
   mIncrement = 1;
   mBPM = 120.0;
   mSetup = false;
   mSecondsTillNextBeat = 1;
}

void Transport::setup(unsigned int sampleRate){
   mSetup = true;
   mSampleRate = sampleRate;
   bpm(mBPM);
}

bool Transport::tick(){
   double index = mPosition.pos_in_beat() + mIncrement;
   if(index >= 1.0){
      while(index >= 1.0){
         index -= 1.0;
         mPosition.advance_beat();
      }
      mPosition.pos_in_beat(index);
      mSecondsTillNextBeat = (1.0 - index) * 60.0 / mBPM;
      return true;
   } else {
      mPosition.pos_in_beat(index);
      mSecondsTillNextBeat = (1.0 - index) * 60.0 / mBPM;
      return false;
   }
}

double Transport::seconds_till_next_beat() const {
   return mSecondsTillNextBeat;
}

const TimePoint& Transport::position() const { return mPosition; }
double Transport::bpm() const { return mBPM; }
double Transport::seconds_per_beat() const { return 60.0 / mBPM; }
int Transport::frames_per_beat() const {
  return static_cast<double>(mSampleRate) * 60.0 / mBPM;
}

void Transport::position(const TimePoint &pos){
   mPosition = pos;
}

void Transport::bpm(double val){
   mBPM = val;
   if(mSetup)
      mIncrement = mBPM / (60.0 * (double)mSampleRate);
}

TransportCommand::TransportCommand(Transport * t){
   mTransport = t;
}

Transport * TransportCommand::transport() const {
   return mTransport;
}

TransportPositionCommand::TransportPositionCommand(
      Transport * t, const TimePoint& pos) :
   TransportCommand(t) 
{ 
   mTimePoint = pos;
}

void TransportPositionCommand::execute(const Transport& /*transport*/){
   if(mTimePoint.valid())
      transport()->position(mTimePoint);
}

bool TransportPositionCommand::store(CommandIOData& /*data*/) const{
   //XXX TODO
   return false;
}

TransportBPMCommand::TransportBPMCommand(Transport * t, double bpm) :
   TransportCommand(t)
{
   mBPM = bpm;
}

void TransportBPMCommand::execute(const Transport& /*transport*/){
   Transport * t = transport();
   if (t)
      t->bpm(mBPM);
}

bool TransportBPMCommand::store(CommandIOData& /*data*/) const{
   //XXX TODO
   return false;
}


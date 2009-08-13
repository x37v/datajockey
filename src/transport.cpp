#include "transport.hpp"

using namespace DataJockey;

Transport::Transport(){
	mPosition.at_bar(0);
}

const TimePoint& Transport::time_point() const { return mPosition; }

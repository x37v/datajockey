#include "beatbuffer.hpp"
#include <yamlcpp/yaml.hpp>
#include <yamlcpp/parser.hpp>

using namespace DataJockey;

template<typename Type>
Type linear_interp(Type v0, Type v1, double dist){
	return v0 + (v1 - v0) * dist;
}

BeatBuffer::BeatBuffer(std::string dataLocation) 
	throw(std::runtime_error){
		mStartBeat = 0;
		load(dataLocation);
}

//XXX assuming 4/4
double BeatBuffer::get_time(const TimePoint position){
	if(TimePoint::SECONDS == position.type())
		return position.seconds();
	else {
		if(position.valid()){
			int beat = position.beat() + position.bar() * 4;
			unsigned int size = mBeatData.size();
			beat += mStartBeat;
			//make sure we're in range!
			if(beat < 0)
				return 0.0;
			if(beat >= size)
				return mBeatData.back();
			else if (position.pos_in_beat() < 0 || 
					(position.pos_in_beat() > 0 && beat + 1 >= size))
				return mBeatData[beat];
			else
				return linear_interp(mBeatData[beat], 
						mBeatData[beat + 1], position.pos_in_beat());
		} else
			return 0.0;
	}
}

TimePoint BeatBuffer::get_position(double seconds){
	TimePoint pos;
	//XXX not implemented yet!
	return pos;
}

TimePoint BeatBuffer::end(){
	TimePoint pos;
	unsigned int beats;
	if(mBeatData.size() == 0)
		return pos;
	//XXX assuming 4/4
	beats = mBeatData.size() - mStartBeat;
	pos.type(TimePoint::BEAT_BAR);
	pos.bar(beats / 4);
	pos.beat(beats % 4);
	pos.pos_in_beat(0);
}

void BeatBuffer::load(std::string dataLocation)
	throw(std::runtime_error)
{
	mBeatData.clear();

	//read in the beat location information
	try {
		yaml::Parser y;
		yaml::node top = y.parseFile(dataLocation);
		yaml::map_ptr beatLocs = yaml::get<yaml::map_ptr>(top);
		yaml::map::iterator it = beatLocs->find(std::string("beat locations"));
		if(it == beatLocs->end())
			throw std::runtime_error("cannot find beat locations");

		//if it is a sequence use the last one for now!
		if(yaml::is<yaml::seq_ptr>(it->second)){
			yaml::seq_ptr beatLocSeq = yaml::get<yaml::seq_ptr>(it->second);

			//store the last
			beatLocs = yaml::get<yaml::map_ptr>( beatLocSeq->back());

			/*
			//this tries to find the one with the most recent mtime..
			beatLocs = yaml::get<yaml::map_ptr>( beatLocSeq->front());
			try {
				//do we have to check the first element?
				for(yaml::seq::iterator seq_it = beatLocSeq->begin();
						seq_it != beatLocSeq->end(); seq_it++){
					//grab the map
					yaml::map_ptr curBeats = yaml::get<yaml::map_ptr>(*seq_it);
					yaml::map::iterator mtime0 = beatLocs->find(std::string("mtime"));
					yaml::map::iterator mtime1 = curBeats->find(std::string("mtime"));;
					//make sure these are valid
					if(mtime0 != beatLocs->end() && mtime1 != curBeats->end()){
						//grab the times
						DateTimePtr t0 = yaml::get<DateTimePtr>(mtime0->second);
						DateTimePtr t1 = yaml::get<DateTimePtr>(mtime1->second);
						//if the beatlocs we're using was modified before the current beat locs, 
						//use the current beat locs
						if(*t0 < *t1)
							beatLocs = curBeats;
					}
				}
			} catch (...){
				//do nothing we've still got the first one set
			}
			*/

		} else 
			beatLocs = yaml::get<yaml::map_ptr>(it->second);

		//grab the time point vector
		it = beatLocs->find(std::string("time points"));
		if(it == beatLocs->end())
			throw std::runtime_error("cannot find time points");

		//copy over the data
		mBeatData = yaml::getVector<double>(it->second);
	} catch (...){
		std::string str("error reading beat location file: ");
		str.append(dataLocation);
		throw std::runtime_error(str);
	}
}

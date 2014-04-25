/*
 *    Copyright (c) 2008 Alex Norman.  All rights reserved.
 *    http://www.x37v.info/datajockey
 *
 *    This file is part of Data Jockey.
 *    
 *    Data Jockey is free software: you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation, either version 3 of the License, or (at your
 *    option) any later version.
 *    
 *    Data Jockey is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *    Public License for more details.
 *    
 *    You should have received a copy of the GNU General Public License along
 *    with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * written by Alex Norman
 * adapted from:
 * vorbisfile_example.c (C) COPYRIGHT 1994-2007
 * by the Xiph.Org Foundation http://www.xiph.org/
 * and
 * madlld.c (c) 2001--2004 Bertrand Petit
 *
 */

#include "soundfile.hpp"
#include "xing.h"
#include <QFileInfo>

#include <stdint.h>
#ifndef INT16_MAX
#define INT16_MAX 32767
#endif

#define MAX_BUF_LEN 2048

#include <iostream>

//forward declarations
static inline signed int madScale(mad_fixed_t sample);

namespace {
  const QString mp3_extension("mp3");
}

//open the soundfile
SoundFile::SoundFile(QString location) : 
  mFile(QFile::encodeName(location)),
  mType(UNSUPPORTED),
  mPCMData(NULL),
  mSampleRate(0),
  mChannels(0),
  mLocation(location)
{
  if (!mFile.open(QIODevice::ReadOnly))
    return;

  //use the given file handle and don't close it when calling sf_close [because mFile will]
  mSndFile = SndfileHandle(mFile.handle(), false);

  //determine what type of soundfile it is
  if(mSndFile && mSndFile.error() == SF_ERR_NO_ERROR){
    mType = SNDFILE;
    mSampleRate = mSndFile.samplerate();
    mChannels = mSndFile.channels();
    //see if it is an mp3
  } else {
    mFile.close();
    QFileInfo file_info(location);
    QString extension = file_info.completeSuffix().toLower();
    if(extension == mp3_extension) {
      mType = MP3;

      //if libsoundfile couldn't open.. see if we can open it as an mp3
      mMP3Data.inputFile.open(QFile::encodeName(location));
      mMP3Data.inputFile.seekg(0, std::ios::end);
      mMP3Data.fileSize = mMP3Data.inputFile.tellg();
      mMP3Data.inputFile.seekg(0, std::ios::beg);

      mMP3Data.inputBuffer = new unsigned char[MAX_BUF_LEN + MAD_BUFFER_GUARD];

      mMP3Data.remaining = 0;
      mMP3Data.endOfFile = false;

      mad_stream_init(&mMP3Data.stream);
      mad_frame_init(&mMP3Data.frame);
      mad_synth_init(&mMP3Data.synth);

      //get the duration, might cause a seek so seek back to the start of the file after
      auto lengthSeconds = getMadDuration();
      mad_frame_mute(&mMP3Data.frame);
      mMP3Data.stream.next_frame = NULL;
      mMP3Data.stream.sync = 0;
      //mMP3Data.stream.error = MAD_ERROR_NONE;
      mMP3Data.stream.error = MAD_ERROR_BUFLEN; //force a buffer read
      mMP3Data.inputFile.seekg(0, std::ios::beg);

      //synth our first frame so that we can get the sample rate
      synthMadFrame();
      mSampleRate = mMP3Data.synth.pcm.samplerate;
      mChannels = mMP3Data.synth.pcm.channels;

      mMP3Data.frameCount = mSampleRate * lengthSeconds;
    }
  }
  mPCMData = new short[1024 * channels()];
}

SoundFile::~SoundFile(){
  //clean up!
  switch(mType){
    case SNDFILE:
      mFile.close();
      break;
    case MP3: 
      mMP3Data.inputFile.close();
      mad_stream_finish(&mMP3Data.stream);
      mad_frame_finish(&mMP3Data.frame);
      mad_synth_finish(&mMP3Data.synth);
      delete [] mMP3Data.inputBuffer;
      break;
    default:
      break;
  };

  if (mPCMData)
    delete [] mPCMData;
}

unsigned int SoundFile::samplerate(){
  return mSampleRate;
}

unsigned int SoundFile::channels(){
  return mChannels;
}

unsigned int SoundFile::readFloatFrame(float *ptr, unsigned int frames){
  int toRead = frames;
  int framesRead;
  float * curBufPtr = ptr;
  if (mChannels == 0) {
    return 0;
  }

  while(toRead > 0){
    if(toRead > 1024)
      framesRead = mp3ReadShortFrame(mPCMData, 1024);
    else
      framesRead = mp3ReadShortFrame(mPCMData, toRead);
    //if we didn't read anything then we're at the end of the file..
    if (framesRead == 0) {
      break;
    } else {
      //XXX deal with sample rate changes
      //convert to float
      for(int i = 0; i < framesRead * (int)mChannels; i++)
        curBufPtr[i] = ((float)mPCMData[i]) / INT16_MAX;
      //inc the current buffer pointer for the next round
      //decrement the number of frames to read next round
      curBufPtr += framesRead * mChannels;
      toRead -= framesRead;
    }
  }
  return frames - toRead;
}

unsigned int SoundFile::mp3ReadShortFrame(short *ptr, unsigned int frames){
  unsigned int framesRead = 0;
  do {
    //first read in any remaining data we have left from last time
    if(mMP3Data.remaining > 0){
      unsigned int nchannels = mMP3Data.synth.pcm.channels;
      unsigned int nsamples;
      unsigned int offset = (mMP3Data.synth.pcm.length - mMP3Data.remaining);
      mad_fixed_t const *left_ch   = mMP3Data.synth.pcm.samples[0] + offset;

      //see how much we should read.. if there are more samples remaining than
      //we need to read then make sure we only read enough..
      if(mMP3Data.remaining + framesRead > frames){
        nsamples = frames - framesRead;
        mMP3Data.remaining = mMP3Data.synth.pcm.length - nsamples;
      } else {
        nsamples = mMP3Data.remaining;
        mMP3Data.remaining = 0;
      }

      //read in our samples
      if(nchannels == 1){
        while(nsamples--){
          ptr[framesRead] = madScale(*left_ch++);
          framesRead++;
        }
      } else {
        mad_fixed_t const *right_ch  = mMP3Data.synth.pcm.samples[1] + offset;
        while(nsamples--){
          ptr[framesRead * 2] = madScale(*left_ch++);
          ptr[framesRead * 2 + 1] = madScale(*right_ch++);
          framesRead++;
        }
      }

    } else if(mMP3Data.endOfFile){
      return framesRead;
    }

    if(framesRead >= frames)
      return frames;
    //get our frame
    synthMadFrame();
  } while (true);
}

unsigned int SoundFile::readf(float *ptr, unsigned int frames){

  //if this file is not valid/supported, we don't return any data
  if(!*this)
    return 0;

  switch(mType){
    case SNDFILE:
      return mSndFile.readf(ptr, frames);
    case MP3:
      return readFloatFrame(ptr,frames);
    default:
      return 0;
  };
}

unsigned int SoundFile::readf(short *ptr, unsigned int frames){

  //if this file is not valid/supported, we don't return any data
  if(!*this)
    return 0;

  switch(mType){
    case SNDFILE:
      return mSndFile.readf(ptr, frames);
    case MP3:
      return mp3ReadShortFrame(ptr,frames);
    default:
      return 0;
  };
}

QString SoundFile::location() const { return mLocation; }

SoundFile::operator bool () const {
  return valid();
}

bool SoundFile::valid() const {
  switch(mType){
    case SNDFILE:
      return true;
    case MP3: 
      if(mSampleRate == 0)
        return false;
      else
        return true;
    default:
      return false;
  };
}

size_t SoundFile::fillMadBuffer() {
  size_t readCount = 0;
  size_t inputRemaining = 0;
  //copy the unprocessed data to the start of our buffer..
  if(mMP3Data.stream.next_frame != NULL){
    inputRemaining = mMP3Data.stream.bufend - mMP3Data.stream.next_frame;
    memmove(mMP3Data.inputBuffer, mMP3Data.stream.next_frame, inputRemaining);
  }

  mMP3Data.inBufLength = 0;

  //grab input data
  for(unsigned int i = inputRemaining; i < MAX_BUF_LEN; i++){
    //read in the data
    int ch;
    ch = mMP3Data.inputFile.get();
    if(EOF == ch){
      //if we're at the end zero out the rest of the input buffer
      memset(mMP3Data.inputBuffer + i, 0, sizeof(unsigned char) * (MAD_BUFFER_GUARD + MAX_BUF_LEN - i));
      mMP3Data.endOfFile = true;
      //we need to have some zeros to be able to decode the last frame
      mMP3Data.inBufLength += MAD_BUFFER_GUARD;
      break;
    }
    mMP3Data.inBufLength = i + 1;
    mMP3Data.inputBuffer[i] = (unsigned char)ch;
    readCount++;
  }
  //decode our data
  mad_stream_buffer(&mMP3Data.stream, mMP3Data.inputBuffer, mMP3Data.inBufLength);
  mMP3Data.stream.error = (mad_error)0;
  return readCount;
}

void SoundFile::synthMadFrame(){
  do {
    if(mMP3Data.stream.buffer == NULL || mMP3Data.stream.error == MAD_ERROR_BUFLEN)
      fillMadBuffer();
    if(mad_frame_decode(&mMP3Data.frame,&mMP3Data.stream)) {
      if(MAD_RECOVERABLE(mMP3Data.stream.error)){
        if(mMP3Data.endOfFile)
          return;
        else
          continue;
      } else {
        if(mMP3Data.stream.error == MAD_ERROR_BUFLEN){
          mMP3Data.remaining = 0;
          if(mMP3Data.endOfFile)
            return;
          else
            continue;
        } else {
          //XXX THROW ERROR!!
          return;
        }
      }
    } 
    //synth the frame!
    mad_synth_frame(&mMP3Data.synth,&mMP3Data.frame);
    //we just synthed the frame, so, we have length remaining to process
    mMP3Data.remaining = mMP3Data.synth.pcm.length;
    return;
  } while(true);
}

//grabbed from moc and modified 2014

/*
 * MOC - music on console
 * Copyright (C) 2002 - 2006 Damian Pietras <daper@daper.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

/* This code was writen od the basis of madlld.c (C) by Bertrand Petit 
 * including code from xmms-mad (C) by Sam Clegg and winamp plugin for madlib
 * (C) by Robert Leslie.
 */
signed long SoundFile::getMadDuration() {
	struct xing xing;
	unsigned long bitrate = 0;
	int has_xing = 0;
	int is_vbr = 0;
	int num_frames = 0;
	mad_timer_t duration = mad_timer_zero;
	struct mad_header header;
	int good_header = 0; /* Have we decoded any header? */

	mad_header_init (&header);
	xing_init (&xing);

	/* There are three ways of calculating the length of an mp3:
	  1) Constant bitrate: One frame can provide the information
		 needed: # of frames and duration. Just see how long it
		 is and do the division.
	  2) Variable bitrate: Xing tag. It provides the number of 
		 frames. Each frame has the same number of samples, so
		 just use that.
	  3) All: Count up the frames and duration of each frames
		 by decoding each one. We do this if we've no other
		 choice, i.e. if it's a VBR file with no Xing tag.
	*/

	while (1) {
		/* Fill the input buffer if needed */
    if (mMP3Data.stream.buffer == NULL || mMP3Data.stream.error == MAD_ERROR_BUFLEN) {
			if (!fillMadBuffer())
				break;
		}

		if (mad_header_decode(&header, &mMP3Data.stream) == -1) {
			if (MAD_RECOVERABLE(mMP3Data.stream.error))
				continue;
			else if (mMP3Data.stream.error == MAD_ERROR_BUFLEN)
				continue;
			else {
				//debug ("Can't decode header: %s", mad_stream_errorstr( &data->stream));
				break;
			}
		}

		good_header = 1;

		/* Limit xing testing to the first frame header */
		if (!num_frames++) {
			if (xing_parse(&xing, mMP3Data.stream.anc_ptr, mMP3Data.stream.anc_bitlen) != -1) {
				is_vbr = 1;
				//debug ("Has XING header");
				
				if (xing.flags & XING_FRAMES) {
					has_xing = 1;
					num_frames = xing.frames;
					break;
				}
				//debug ("XING header doesn't contain number of " "frames.");
			}
		}				

		/* Test the first n frames to see if this is a VBR file */
		if (!is_vbr && !(num_frames > 20)) {
			if (bitrate && header.bitrate != bitrate) {
				//debug ("Detected VBR after %d frames", num_frames);
				is_vbr = 1;
			} else
				bitrate = header.bitrate;
		}
		
		/* We have to assume it's not a VBR file if it hasn't already
		 * been marked as one and we've checked n frames for different
		 * bitrates */
		else if (!is_vbr) {
			//debug ("Fixed rate MP3");
			break;
		}
			
		mad_timer_add (&duration, header.duration);
	}

	if (!good_header)
		return -1;

	if (!is_vbr) {
		/* time in seconds */
		double time = (mMP3Data.fileSize * 8.0) / (header.bitrate);
		double timefrac = (double)time - ((long)(time));

		/* samples per frame */
		long nsamples = 32 * MAD_NSBSAMPLES(&header);
		/* samplerate is a constant */
		num_frames = (long) (time * header.samplerate / nsamples);
		mad_timer_set(&duration, (long)time, (long)(timefrac*100), 100);
	} else if (has_xing) {
		mad_timer_multiply (&header.duration, num_frames);
		duration = header.duration;
	}
	else {
		/* the durations have been added up, and the number of frames
		   counted. We do nothing here. */
		//debug ("Counted duration by counting frames durations in "
				//"VBR file.");
	}

	mad_header_finish(&header);
	//debug ("MP3 time: %ld", mad_timer_count (duration, MAD_UNITS_SECONDS));

	return mad_timer_count(duration, MAD_UNITS_SECONDS);
}

unsigned int SoundFile::frames() {
  switch(mType) {
    case SNDFILE:
      return (unsigned int)mSndFile.frames();
    case MP3:
      if (mMP3Data.frameCount < 0)
        return 0;
      return (unsigned int)mMP3Data.frameCount;
    default:
    case UNSUPPORTED:
      return 0;
  }
}

//MAD stuff
static inline signed int madScale(mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}


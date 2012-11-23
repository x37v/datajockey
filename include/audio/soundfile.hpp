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

#ifndef ALEX_SOUNDFILE_HPP
#define ALEX_SOUNDFILE_HPP

#include <sndfile.hh>
#include <string>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <mad.h>
#include <fstream>
#include <vector>

class SoundFile {
  public:
    typedef struct {

      struct mad_stream stream;
      struct mad_frame  frame;
      struct mad_synth  synth;

      unsigned char *inputBuffer;
      unsigned long inBufLength;
      std::ifstream inputFile;
      
      bool endOfFile;

      //this is a count of unread frames remaining in synth.pcm
      unsigned int remaining;
    } MP3FileData;
  private:
    enum filetype {UNSUPPORTED, SNDFILE, OGG, MP3};

    SndfileHandle mSndFile;
    filetype mType;
    OggVorbis_File mOggFile;
    int mOggIndex;
    MP3FileData mMP3Data;

    void synthMadFrame();
    int mSampleRate;
    unsigned int mChannels;

    //private member functions for reading ogg/mp3 shorts
    unsigned int oggReadShortFrame (short *ptr, unsigned int frames);
    unsigned int mp3ReadShortFrame (short *ptr, unsigned int frames);
    //this will read either mp3 or ogg float frames
    unsigned int readFloatFrame (float *ptr, unsigned int frames);
  public:
    SoundFile(std::string location);
    ~SoundFile();
    unsigned int samplerate();
    unsigned int channels();
    unsigned int readf (float *ptr, unsigned int frames) ;
    unsigned int readf (short *ptr, unsigned int frames) ;
    operator bool () const ;
      bool valid() const;
      unsigned int frames();
};

#endif


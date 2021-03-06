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
#include <QString>
#include <QFile>
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

      signed long frameCount;

      size_t fileSize;

      bool endOfFile;

      //this is a count of unread frames remaining in synth.pcm
      unsigned int remaining;
    } MP3FileData;
  private:
    enum filetype {UNSUPPORTED, SNDFILE, MP3};

    QFile mFile;
    SndfileHandle mSndFile;
    filetype mType;
    MP3FileData mMP3Data;

    short * mPCMData;

    void synthMadFrame();
    size_t fillMadBuffer();
    signed long getMadDuration();

    int mSampleRate;
    unsigned int mChannels;
    QString mLocation;

    //private member functions for reading mp3 shorts
    unsigned int mp3ReadShortFrame (short *ptr, unsigned int frames);
    //this will read mp3 float frames
    unsigned int readFloatFrame (float *ptr, unsigned int frames);
  public:
    SoundFile(QString location);
    ~SoundFile();
    unsigned int samplerate();
    unsigned int channels();
    unsigned int readf (float *ptr, unsigned int frames) ;
    unsigned int readf (short *ptr, unsigned int frames) ;
    QString location() const;
    operator bool () const ;
    bool valid() const;
    unsigned int frames() const;
    double seconds() const;
};

#endif


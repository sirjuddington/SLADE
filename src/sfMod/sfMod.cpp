////////////////////////////////
// sfmod 1.1.0                //
// Copyright © Kerli Low 2012 //
////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// License:                                                                 //
// sfmod                                                                    //
// Copyright (c) 2012 Kerli Low                                             //
// kerlilow@gmail.com                                                       //
//                                                                          //
// This software is provided 'as-is', without any express or implied        //
// warranty. In no event will the authors be held liable for any damages    //
// arising from the use of this software.                                   //
//                                                                          //
// Permission is granted to anyone to use this software for any purpose,    //
// including commercial applications, and to alter it and redistribute it   //
// freely, subject to the following restrictions:                           //
//                                                                          //
//    1. The origin of this software must not be misrepresented; you must   //
//    not claim that you wrote the original software. If you use this       //
//    software in a product, an acknowledgment in the product documentation //
//    would be appreciated but is not required.                             //
//                                                                          //
//    2. Altered source versions must be plainly marked as such, and must   //
//    not be misrepresented as being the original software.                 //
//                                                                          //
//    3. This notice may not be removed or altered from any source          //
//    distribution.                                                         //
//////////////////////////////////////////////////////////////////////////////

#ifndef NOLIBMODPLUG

#include "sfMod.h"
#include "modplug.h"

#include <fstream>



namespace sfmod
{
  ModPlug_Settings settings;
  ModPlug_Settings defSettings;
}


void sfmod::initSettings()
{
  ModPlug_GetSettings(&sfmod::defSettings);

  sfmod::settings = sfmod::defSettings;
}

void sfmod::applySettings()
{
  ModPlug_SetSettings(&sfmod::settings);
}

void sfmod::defaultSettings()
{
  ModPlug_SetSettings(&sfmod::defSettings);

  sfmod::settings = sfmod::defSettings;
}


///////////////////////////////////////////////////////////
// Mod class                                             //
///////////////////////////////////////////////////////////
sfmod::Mod::Mod() : sf::SoundStream(), sfmod::Error()
{
  file_ = NULL;

  name_   = "";
  length_ = 0;

  buffer_.resize(SFMOD_BUFFERSIZE / 2);
}

sfmod::Mod::Mod(const std::string& filename)
  : sf::SoundStream(), sfmod::Error()
{
  file_ = NULL;

  name_   = "";
  length_ = 0;

  buffer_.resize(SFMOD_BUFFERSIZE / 2);

  loadFromFile(filename);
}

sfmod::Mod::Mod(const void* data, unsigned int size)
  : sf::SoundStream(), sfmod::Error()
{
  file_ = NULL;

  name_   = "";
  length_ = 0;

  buffer_.resize(SFMOD_BUFFERSIZE / 2);

  loadFromMemory(data, size);
}


sfmod::Mod::~Mod()
{
  unload();
}


bool sfmod::Mod::loadFromFile(const std::string& filename)
{
  unload();

  std::string data;

  std::ifstream file;
  file.open(filename.c_str(), std::ios::binary);
  if (!file.is_open()) {
    setError("Failed to load module.");
    return false;
  }

  file.seekg(0, std::ios::end);
  unsigned int size = static_cast<unsigned int>(file.tellg());
  file.seekg(0, std::ios::beg);

  data.reserve(size);
  for (unsigned int i = 0; i < size; ++i)
    data += file.get();

  file.close();

  file_ = ModPlug_Load(data.c_str(), size);
  if (file_ == NULL) {
    setError("Failed to load module.");
    return false;
  }

  name_   = ModPlug_GetName(file_);
  length_ = ModPlug_GetLength(file_);

  ModPlug_Settings settings;
  ModPlug_GetSettings(&settings);

  initialize(settings.mChannels, settings.mFrequency);

  return true;
}

bool sfmod::Mod::loadFromMemory(const std::string& data)
{
  unload();

  file_ = ModPlug_Load(data.c_str(), data.size());
  if (file_ == NULL) {
    setError("Failed to load module.");
    return false;
  }

  name_   = ModPlug_GetName(file_);
  length_ = ModPlug_GetLength(file_);

  ModPlug_Settings settings;
  ModPlug_GetSettings(&settings);

  initialize(settings.mChannels, settings.mFrequency);

  return true;
}

bool sfmod::Mod::loadFromMemory(const void* data, unsigned int size)
{
  unload();

  file_ = ModPlug_Load(data, size);
  if (file_ == NULL) {
    setError("Failed to load module.");
    return false;
  }

  name_   = ModPlug_GetName(file_);
  length_ = ModPlug_GetLength(file_);

  ModPlug_Settings settings;
  ModPlug_GetSettings(&settings);

  initialize(settings.mChannels, settings.mFrequency);

  return true;
}

void sfmod::Mod::unload()
{
  if (getStatus() != sf::SoundStream::Stopped)
    stop();

  if (file_ != NULL) {
    ModPlug_Unload(file_);
    file_ = NULL;
  }

  name_   = "";
  length_ = 0;

  buffer_.clear();
  buffer_.resize(SFMOD_BUFFERSIZE / 2);
}


ModPlugFile* sfmod::Mod::getModPlugFile() const
{
  return file_;
}

const std::string& sfmod::Mod::getName() const
{
  return name_;
}

int sfmod::Mod::getLength() const
{
  return length_;
}


int sfmod::Mod::getModuleType() const
{
  return ModPlug_GetModuleType(file_);
}

std::string sfmod::Mod::getSongComments() const
{
  char* comments = ModPlug_GetMessage(file_);
  if (comments == NULL)
    return std::string();

  return std::string(comments);
}


unsigned int sfmod::Mod::getMasterVolume() const
{
  return ModPlug_GetMasterVolume(file_);
}


int sfmod::Mod::getCurrentSpeed() const
{
  return ModPlug_GetCurrentSpeed(file_);
}

int sfmod::Mod::getCurrentTempo() const
{
  return ModPlug_GetCurrentTempo(file_);
}

int sfmod::Mod::getCurrentOrder() const
{
  return ModPlug_GetCurrentOrder(file_);
}

int sfmod::Mod::getCurrentPattern() const
{
  return ModPlug_GetCurrentPattern(file_);
}

int sfmod::Mod::getCurrentRow() const
{
  return ModPlug_GetCurrentRow(file_);
}

int sfmod::Mod::getPlayingChannels() const
{
  return ModPlug_GetPlayingChannels(file_);
}


unsigned int sfmod::Mod::getInstrumentCount() const
{
  return ModPlug_NumInstruments(file_);
}

unsigned int sfmod::Mod::getSampleCount() const
{
  return ModPlug_NumSamples(file_);
}

unsigned int sfmod::Mod::getPatternCount() const
{
  return ModPlug_NumPatterns(file_);
}

unsigned int sfmod::Mod::getChannelCount() const
{
  return ModPlug_NumChannels(file_);
}


std::string sfmod::Mod::getInstrumentName(unsigned int index) const
{
  char buf[40] = {0};

  ModPlug_InstrumentName(file_, index, buf);

  return std::string(buf);
}

std::string sfmod::Mod::getSampleName(unsigned int index) const
{
  char buf[40] = {0};

  ModPlug_SampleName(file_, index, buf);

  return std::string(buf);
}


ModPlugNote* sfmod::Mod::getPattern(int pattern, unsigned int* numrows) const
{
  return ModPlug_GetPattern(file_, pattern, numrows);
}


void sfmod::Mod::setMasterVolume(unsigned int volume)
{
  ModPlug_SetMasterVolume(file_, volume);
}


void sfmod::Mod::seekOrder(int order)
{
  ModPlug_SeekOrder(file_, order);
}


void sfmod::Mod::initMixerCallback(ModPlugMixerProc proc)
{
  ModPlug_InitMixerCallback(file_, proc);
}

void sfmod::Mod::unloadMixerCallback()
{
  ModPlug_UnloadMixerCallback(file_);
}


bool sfmod::Mod::onGetData(sf::SoundStream::Chunk& data)
{
  int read = ModPlug_Read(file_, reinterpret_cast<void*>(&buffer_[0]),
                          SFMOD_BUFFERSIZE);

  if (read == 0)
    return false;

  data.sampleCount = static_cast<size_t>(read / 2);
  data.samples     = &buffer_[0];

  return true;
}

void sfmod::Mod::onSeek(sf::Time timeOffset)
{
  ModPlug_Seek(file_, static_cast<int>(timeOffset.asMilliseconds()));
}

#endif//NOLIBMODPLUG

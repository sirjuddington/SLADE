////////////////////////////////
// sfMod 1.0.2                //
// Copyright © Kerli Low 2012 //
////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// License:                                                                 //
// sfMod                                                                    //
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
//    notbe misrepresented as being the original software.                  //
//                                                                          //
//    3. This notice may not be removed or altered from any source          //
//    distribution.                                                         //
//////////////////////////////////////////////////////////////////////////////

#ifndef NOLIBMODPLUG

#include "sfMod.h"
#include "modplug.h"

#include <fstream>



namespace sfMod
{
  ModPlug_Settings settings;
  ModPlug_Settings defaultSettings;
}


void sfMod::InitSettings()
{
  ModPlug_GetSettings(&sfMod::defaultSettings);

  sfMod::settings = sfMod::defaultSettings;
}

void sfMod::ApplySettings()
{
  ModPlug_SetSettings(&sfMod::settings);
}

void sfMod::DefaultSettings()
{
  ModPlug_SetSettings(&sfMod::defaultSettings);

  sfMod::settings = sfMod::defaultSettings;
}


///////////////////////////////////////////////////////////
// Mod class                                             //
///////////////////////////////////////////////////////////
sfMod::Mod::Mod() : SoundStream()
{
  file_ = NULL;

  name_   = "";
  length_ = 0;

  buffer_.resize(SFMOD_BUFFERSIZE / 2);
}

sfMod::Mod::Mod(const std::string& filename) : SoundStream()
{
  file_ = NULL;

  name_   = "";
  length_ = 0;

  buffer_.resize(SFMOD_BUFFERSIZE / 2);

  LoadFromFile(filename);
}

sfMod::Mod::Mod(const void* data, unsigned int size) : SoundStream()
{
  file_ = NULL;

  name_   = "";
  length_ = 0;

  buffer_.resize(SFMOD_BUFFERSIZE / 2);

  LoadFromMemory(data, size);
}


sfMod::Mod::~Mod()
{
  Release();
}


void sfMod::Mod::Release()
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


bool sfMod::Mod::LoadFromFile(const std::string& filename)
{
  if (getStatus() != sf::SoundStream::Stopped)
    stop();

  if (file_ != NULL) {
    ModPlug_Unload(file_);
    file_ = NULL;
  }

  name_   = "";
  length_ = 0;

  std::string data;

  std::ifstream file;
  file.open(filename.c_str(), std::ios::binary);
  if (!file.is_open())
    return false;

  file.seekg(0, std::ios::end);
  unsigned int size = static_cast<unsigned int>(file.tellg());
  file.seekg(0, std::ios::beg);

  data.reserve(size);
  for (unsigned int i = 0; i < size; ++i)
    data += file.get();

  file.close();

  file_ = ModPlug_Load(data.c_str(), size);
  if (file_ == NULL)
    return false;

  name_   = ModPlug_GetName(file_);
  length_ = ModPlug_GetLength(file_);

  ModPlug_Settings settings;
  ModPlug_GetSettings(&settings);

  initialize(settings.mChannels, settings.mFrequency);

  return true;
}

bool sfMod::Mod::LoadFromMemory(const std::string& data)
{
  if (getStatus() != sf::SoundStream::Stopped)
    stop();

  if (file_ != NULL) {
    ModPlug_Unload(file_);
    file_ = NULL;
  }

  name_   = "";
  length_ = 0;

  file_ = ModPlug_Load(data.c_str(), data.size());
  if (file_ == NULL)
    return false;

  name_   = ModPlug_GetName(file_);
  length_ = ModPlug_GetLength(file_);

  ModPlug_Settings settings;
  ModPlug_GetSettings(&settings);

  initialize(settings.mChannels, settings.mFrequency);

  return true;
}

bool sfMod::Mod::LoadFromMemory(const void* data, unsigned int size)
{
  if (getStatus() != sf::SoundStream::Stopped)
    stop();

  if (file_ != NULL) {
    ModPlug_Unload(file_);
    file_ = NULL;
  }

  name_   = "";
  length_ = 0;

  file_ = ModPlug_Load(data, size);
  if (file_ == NULL)
    return false;

  name_   = ModPlug_GetName(file_);
  length_ = ModPlug_GetLength(file_);

  ModPlug_Settings settings;
  ModPlug_GetSettings(&settings);

  initialize(settings.mChannels, settings.mFrequency);

  return true;
}


ModPlugFile* sfMod::Mod::GetModPlugFile() const
{
  return file_;
}

const std::string& sfMod::Mod::GetName() const
{
  return name_;
}

int sfMod::Mod::GetLength() const
{
  return length_;
}


int sfMod::Mod::GetModuleType() const
{
  return ModPlug_GetModuleType(file_);
}

std::string sfMod::Mod::GetSongComments() const
{
  char* comments = ModPlug_GetMessage(file_);
  if (comments == NULL)
    return std::string();

  return std::string(comments);
}


unsigned int sfMod::Mod::GetMasterVolume() const
{
  return ModPlug_GetMasterVolume(file_);
}


int sfMod::Mod::GetCurrentSpeed() const
{
  return ModPlug_GetCurrentSpeed(file_);
}

int sfMod::Mod::GetCurrentTempo() const
{
  return ModPlug_GetCurrentTempo(file_);
}

int sfMod::Mod::GetCurrentOrder() const
{
  return ModPlug_GetCurrentOrder(file_);
}

int sfMod::Mod::GetCurrentPattern() const
{
  return ModPlug_GetCurrentPattern(file_);
}

int sfMod::Mod::GetCurrentRow() const
{
  return ModPlug_GetCurrentRow(file_);
}

int sfMod::Mod::GetPlayingChannels() const
{
  return ModPlug_GetPlayingChannels(file_);
}


unsigned int sfMod::Mod::GetInstrumentCount() const
{
  return ModPlug_NumInstruments(file_);
}

unsigned int sfMod::Mod::GetSampleCount() const
{
  return ModPlug_NumSamples(file_);
}

unsigned int sfMod::Mod::GetPatternCount() const
{
  return ModPlug_NumPatterns(file_);
}

unsigned int sfMod::Mod::GetChannelCount() const
{
  return ModPlug_NumChannels(file_);
}


std::string sfMod::Mod::GetInstrumentName(unsigned int index) const
{
  char buf[40] = {0};

  ModPlug_InstrumentName(file_, index, buf);

  return std::string(buf);
}

std::string sfMod::Mod::GetSampleName(unsigned int index) const
{
  char buf[40] = {0};

  ModPlug_SampleName(file_, index, buf);

  return std::string(buf);
}


ModPlugNote* sfMod::Mod::GetPattern(int pattern, unsigned int* numrows) const
{
  return ModPlug_GetPattern(file_, pattern, numrows);
}


void sfMod::Mod::SetMasterVolume(unsigned int volume)
{
  ModPlug_SetMasterVolume(file_, volume);
}


void sfMod::Mod::SeekOrder(int order)
{
  ModPlug_SeekOrder(file_, order);
}


void sfMod::Mod::InitMixerCallback(ModPlugMixerProc proc)
{
  ModPlug_InitMixerCallback(file_, proc);
}

void sfMod::Mod::UnloadMixerCallback()
{
  ModPlug_UnloadMixerCallback(file_);
}

bool sfMod::Mod::onGetData(sf::SoundStream::Chunk& data)
{
  int read = ModPlug_Read(file_, reinterpret_cast<void*>(&buffer_[0]),
                          SFMOD_BUFFERSIZE);

  if (read == 0)
    return false;


  data.sampleCount = static_cast<size_t>(read / 2);
  data.samples = &buffer_[0];

  return true;
}

void sfMod::Mod::onSeek(sf::Time timeOffset)
{
  ModPlug_Seek(file_, static_cast<int>(timeOffset.asMilliseconds()));
}

#endif//NOLIBMODPLUG

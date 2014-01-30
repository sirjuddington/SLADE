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

#ifndef SFMOD_H
#define SFMOD_H

#ifndef NOLIBMODPLUG

#define SFMOD_BUFFERSIZE 4096

#include <string>
#include <vector>
#include <SFML/Audio.hpp>

#include "sfMod/Error.h"



// Forward declarations //
struct _ModPlugFile;
typedef struct _ModPlugFile ModPlugFile;
struct _ModPlugNote;
typedef struct _ModPlugNote ModPlugNote;
struct _ModPlug_Settings;
typedef struct _ModPlug_Settings ModPlug_Settings;
typedef void (*ModPlugMixerProc)(int*, unsigned long, unsigned long);
// //



namespace sfmod
{
  class Mod;

  // See bottom of file for more information on settings
  extern ModPlug_Settings settings;

  void initSettings   ();
  void applySettings  ();
  void defaultSettings();
}


///////////////////////////////////////////////////////////
// Mod class                                             //
///////////////////////////////////////////////////////////
class sfmod::Mod : public sf::SoundStream, public sfmod::Error
{
public:
  Mod();
  Mod(const std::string& filename);
  Mod(const void* data, unsigned int size);

  ~Mod();

  bool loadFromFile  (const std::string& filename);
  bool loadFromMemory(const std::string& data);
  bool loadFromMemory(const void* data, unsigned int size);
  void unload        ();

  ModPlugFile*       getModPlugFile() const;
  const std::string& getName       () const;
  int                getLength     () const;

  // See bottom of file for a list of possible module types
  int         getModuleType  () const;
  std::string getSongComments() const;

  unsigned int getMasterVolume() const; // Range: 1-512

  int getCurrentSpeed   () const;
  int getCurrentTempo   () const;
  int getCurrentOrder   () const;
  int getCurrentPattern () const;
  int getCurrentRow     () const;
  int getPlayingChannels() const;

  unsigned int getInstrumentCount() const;
  unsigned int getSampleCount    () const;
  unsigned int getPatternCount   () const;
  unsigned int getChannelCount   () const;

  std::string getInstrumentName(unsigned int index) const;
  std::string getSampleName    (unsigned int index) const;

  // See bottom of file for more info on ModPlugNote
  ModPlugNote* getPattern(int pattern, unsigned int* numrows) const;

  void setMasterVolume(unsigned int volume); // Range: 1-512

  void seekOrder(int order);

  // See bottom of file for more info on mixer callbacks
  void initMixerCallback  (ModPlugMixerProc proc);
  void unloadMixerCallback();


private:
  bool onGetData(sf::SoundStream::Chunk& data);
  void onSeek   (sf::Time timeOffset);


  ModPlugFile* file_;

  std::string name_;
  int         length_;

  std::vector<sf::Int16> buffer_;
};


///////////////////////////////////////////////////////////
// Settings (Copied from modplug.h for reference)        //
///////////////////////////////////////////////////////////
//
// enum _ModPlug_Flags
// {
// 	MODPLUG_ENABLE_OVERSAMPLING     = 1 << 0,  /* Enable oversampling
//                                              * (*highly* recommended) */
// 	MODPLUG_ENABLE_NOISE_REDUCTION  = 1 << 1,  /* Enable noise reduction */
// 	MODPLUG_ENABLE_REVERB           = 1 << 2,  /* Enable reverb */
// 	MODPLUG_ENABLE_MEGABASS         = 1 << 3,  /* Enable megabass */
// 	MODPLUG_ENABLE_SURROUND         = 1 << 4   /* Enable surround sound. */
// };
// 
// enum _ModPlug_ResamplingMode
// {
// 	MODPLUG_RESAMPLE_NEAREST = 0,  /* No interpolation (very fast,
//                                  * extremely bad sound quality) */
// 	MODPLUG_RESAMPLE_LINEAR  = 1,  /* Linear interpolation
//                                  * (fast, good quality) */
// 	MODPLUG_RESAMPLE_SPLINE  = 2,  /* Cubic spline interpolation
//                                  * (high quality) */
// 	MODPLUG_RESAMPLE_FIR     = 3   /* 8-tap fir filter
//                                  * (extremely high quality) */
// };
// 
// typedef struct _ModPlug_Settings
// {
// 	int mFlags;  /* One or more of the MODPLUG_ENABLE_* flags
//                * above, bitwise-OR'ed */
// 	
// 	/* Note that ModPlug always decodes sound at 44100kHz, 32 bit,
// 	 * stereo and then down-mixes to the settings you choose. */
// 	int mChannels;       /* Number of channels - 1 for mono or 2 for stereo */
// 	int mBits;           /* Bits per sample - 8, 16, or 32 */
// 	int mFrequency;      /* Sampling rate - 11025, 22050, or 44100 */
// 	int mResamplingMode; /* One of MODPLUG_RESAMPLE_*, above */
// 
// 	int mStereoSeparation; /* Stereo separation, 1 - 256 */
// 	int mMaxMixChannels; /* Maximum number of mixing channels (polyphony),
//                        * 32 - 256 */
// 	
// 	int mReverbDepth;    /* Reverb level 0(quiet)-100(loud)      */
// 	int mReverbDelay;    /* Reverb delay in ms, usually 40-200ms */
// 	int mBassAmount;     /* XBass level 0(quiet)-100(loud)       */
// 	int mBassRange;      /* XBass cutoff in Hz 10-100            */
// 	int mSurroundDepth;  /* Surround level 0(quiet)-100(heavy)   */
// 	int mSurroundDelay;  /* Surround delay in ms, usually 5-40ms */
// 	int mLoopCount;      /* Number of times to loop.  Zero prevents looping.
// 	                        -1 loops forever. */
// } ModPlug_Settings;
// 
// /* Get and set the mod decoder settings.  All options, except for channels,
//  * bits-per-sample, sampling rate, and loop count, will take effect
//  * immediately.  Those options which don'ttake effect immediately will
//  * take effect the next time you load a mod. */
//
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// Possible module types                                 //
///////////////////////////////////////////////////////////
//
// MOD_TYPE_NONE
// MOD_TYPE_MOD
// MOD_TYPE_S3M
// MOD_TYPE_XM
// MOD_TYPE_MED
// MOD_TYPE_MTM
// MOD_TYPE_IT
// MOD_TYPE_669
// MOD_TYPE_ULT
// MOD_TYPE_STM
// MOD_TYPE_FAR
// MOD_TYPE_WAV
// MOD_TYPE_AMF
// MOD_TYPE_AMS
// MOD_TYPE_DSM
// MOD_TYPE_MDL
// MOD_TYPE_OKT
// MOD_TYPE_MID
// MOD_TYPE_DMF
// MOD_TYPE_PTM
// MOD_TYPE_DBM
// MOD_TYPE_MT2
// MOD_TYPE_AMF0
// MOD_TYPE_PSM
// MOD_TYPE_J2B
// MOD_TYPE_ABC
// MOD_TYPE_PAT
// MOD_TYPE_UMX
//
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// Mixer callbacks (Copied from modplug.h for reference) //
///////////////////////////////////////////////////////////
//
// Use this callback if you want to 'modify' the mixed data of LibModPlug.
//
// void proc(int* buffer,unsigned long channels,unsigned long nsamples) ;
//
// 'buffer': A buffer of mixed samples
// 'channels': N. of channels in the buffer
// 'nsamples': N. of samples in the buffeer (without taking care of n.channels)
//
// (Samples are signed 32-bit integers)
//
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// ModPlugNote (Copied from modplug.h for reference)     //
///////////////////////////////////////////////////////////
//
// struct _ModPlugNote {
// 	unsigned char Note;
// 	unsigned char Instrument;
// 	unsigned char VolumeEffect;
// 	unsigned char Effect;
// 	unsigned char Volume;
// 	unsigned char Parameter;
// };
// typedef struct _ModPlugNote ModPlugNote;
//
//////////////////////////////////////////////////////////////////////////////

#endif//NOLIBMODPLUG

#endif //SFMOD_H

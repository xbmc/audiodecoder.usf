/*
 *      Copyright (C) 2014 Arne Morten Kvarving
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/Filesystem.h>
#include <iostream>

extern "C" {
#include "usf.h"
#include "psflib.h"
#include <stdio.h>
#include <stdint.h>

struct USFContext {
  char* state = nullptr;
  int64_t len;
  int sample_rate;
  int64_t pos;
  std::string title;
  std::string artist;
};

int usf_load(void* ctx, const uint8_t*, size_t exe_size,
             const uint8_t* data, size_t size)
{
  if (exe_size > 0)
    return -1;
  usf_upload_section(ctx, data, size);
  return 0;
}

static void * psf_file_fopen( const char * uri )
{
  kodi::vfs::CFile* file = new kodi::vfs::CFile;
  if (!file->OpenFile(uri, 0))
  {
    delete file;
    return nullptr;
  }

  return file;
}

static size_t psf_file_fread( void * buffer, size_t size, size_t count, void * handle )
{
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(handle);
  return file->Read(buffer, size*count);
}

static int psf_file_fseek( void * handle, int64_t offset, int whence )
{
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(handle);
  return file->Seek(offset, whence) > -1 ? 0 : -1;
}

static int psf_file_fclose( void * handle )
{
  delete static_cast<kodi::vfs::CFile*>(handle);

  return 0;
}

static long psf_file_ftell( void * handle )
{
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(handle);
  return file->GetPosition();
}

const psf_file_callbacks psf_file_system =
{
  "\\/",
  psf_file_fopen,
  psf_file_fread,
  psf_file_fseek,
  psf_file_fclose,
  psf_file_ftell
};

#define BORK_TIME 0xC0CAC01A
static unsigned long parse_time_crap(const char *input)
{
  if (!input) return BORK_TIME;
  int len = strlen(input);
  if (!len) return BORK_TIME;
  int value = 0;
  {
    int i;
    for (i = len - 1; i >= 0; i--)
    {
      if ((input[i] < '0' || input[i] > '9') && input[i] != ':' && input[i] != ',' && input[i] != '.')
      {
        return BORK_TIME;
      }
    }
  }
  std::string foo = input;
  char *bar = (char *) &foo[0];
  char *strs = bar + foo.size() - 1;
  while (strs > bar && (*strs >= '0' && *strs <= '9'))
  {
    strs--;
  }
  if (*strs == '.' || *strs == ',')
  {
    // fraction of a second
    strs++;
    if (strlen(strs) > 3) strs[3] = 0;
    value = atoi(strs);
    switch (strlen(strs))
    {
      case 1:
        value *= 100;
        break;
      case 2:
        value *= 10;
        break;
    }
    strs--;
    *strs = 0;
    strs--;
  }
  while (strs > bar && (*strs >= '0' && *strs <= '9'))
  {
    strs--;
  }
  // seconds
  if (*strs < '0' || *strs > '9') strs++;
  value += atoi(strs) * 1000;
  if (strs > bar)
  {
    strs--;
    *strs = 0;
    strs--;
    while (strs > bar && (*strs >= '0' && *strs <= '9'))
    {
      strs--;
    }
    if (*strs < '0' || *strs > '9') strs++;
    value += atoi(strs) * 60000;
    if (strs > bar)
    {
      strs--;
      *strs = 0;
      strs--;
      while (strs > bar && (*strs >= '0' && *strs <= '9'))
      {
        strs--;
      }
      value += atoi(strs) * 3600000;
    }
  }
  return value;
}

static int psf_info_meta(void* context,
                         const char* name, const char* value)
{
  USFContext* usf = (USFContext*)context;
  if (!strcasecmp(name, "length"))
    usf->len = parse_time_crap(value);
  if (!strcasecmp(name, "title"))
    usf->title = value;
  if (!strcasecmp(name, "artist"))
    usf->artist = value;

  return 0;
}

}

class ATTRIBUTE_HIDDEN CUSFCodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CUSFCodec(KODI_HANDLE instance) :
    CInstanceAudioDecoder(instance) {}

  virtual ~CUSFCodec()
  {
    delete[] ctx.state;
  }

  virtual bool Init(const std::string& filename, unsigned int filecache,
                    int& channels, int& samplerate,
                    int& bitspersample, int64_t& totaltime,
                    int& bitrate, AEDataFormat& format,
                    std::vector<AEChannel>& channellist) override
  {
    ctx.pos = 0;
    ctx.state = new char[usf_get_state_size()];
    usf_clear(ctx.state);
    if (psf_load(filename.c_str(), &psf_file_system, 0x21, 0, 0,
                 psf_info_meta, &ctx, 0) <= 0)
    {
      delete ctx.state;
      return false;
    }
    if (psf_load(filename.c_str(), &psf_file_system, 0x21, usf_load,
                 ctx.state, 0, 0, 0) < 0)
    {
      delete ctx.state;
      return false;
    }
    totaltime = ctx.len;
    usf_set_compare(ctx.state, 0);
    usf_set_fifo_full(ctx.state, 0);
    usf_set_hle_audio(ctx.state, 1);
    format = AE_FMT_S16NE;
    channellist = { AE_CH_FL, AE_CH_FR };
    channels = 2;

    bitspersample = 16;
    bitrate = 0.0;

    int32_t srate;
    usf_render(ctx.state, NULL, 0, &srate);
    samplerate = ctx.sample_rate = srate;

    ctx.len = srate*4*(totaltime)/1000;

    return true;
  }

  virtual int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    if (ctx.pos >= ctx.len)
      return 1;
    if (usf_render(ctx.state, (int16_t*)buffer, size/4, &ctx.sample_rate))
      return 1;
    ctx.pos += size;
    actualsize = size;
    return 0;
  }

  virtual int64_t Seek(int64_t time) override
  {
    if (time*ctx.sample_rate*4/1000 < ctx.pos)
    {
      usf_restart(ctx.state);
      ctx.pos = 0;
    }

    int64_t left = time*ctx.sample_rate*4/1000-ctx.pos;
    while (left > 4096)
    {
      usf_render(ctx.state, nullptr, 1024, &ctx.sample_rate);
      ctx.pos += 4096;
      left -= 4096;
    }

    return ctx.pos/(ctx.sample_rate*4)*1000;
  }

  virtual bool ReadTag(const std::string& file, std::string& title,
                       std::string& artist, int& length) override
  {
    USFContext usf;
    ctx.state = new char[usf_get_state_size()];
    usf_clear(ctx.state);

    if (psf_load(file.c_str(), &psf_file_system, 0x21, 0,
                0, psf_info_meta, &usf, 0) <= 0)
    {
      return false;
    }

    title = usf.title;
    artist = usf.artist;
    length = usf.len/1000;

    delete[] usf.state;
    return true;
  }

private:
  USFContext ctx;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CUSFCodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon()
  {
  }
};


ADDONCREATOR(CMyAddon);

#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/sox.c"
#else

/* ---------------------------------------------------------------------- */
/* -- */
/* -- Copyright (c) 2012 Soumith Chintala */
/* --  */
/* -- Permission is hereby granted, free of charge, to any person obtaining */
/* -- a copy of this software and associated documentation files (the */
/* -- "Software"), to deal in the Software without restriction, including */
/* -- without limitation the rights to use, copy, modify, merge, publish, */
/* -- distribute, sublicense, and/or sell copies of the Software, and to */
/* -- permit persons to whom the Software is furnished to do so, subject to */
/* -- the following conditions: */
/* --  */
/* -- The above copyright notice and this permission notice shall be */
/* -- included in all copies or substantial portions of the Software. */
/* --  */
/* -- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, */
/* -- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF */
/* -- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND */
/* -- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE */
/* -- LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION */
/* -- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION */
/* -- WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
/* --  */
/* ---------------------------------------------------------------------- */
/* -- description: */
/* --     sox.c - a wrapper from libSox to Torch-7 */
/* -- */
/* -- history:  */
/* --     May 24th, 2012, 8:38PM - wrote load function - Soumith Chintala */
/* ---------------------------------------------------------------------- */

typedef struct
{
    THTensor *t;
    int       nChannels;
    long      bufferSize;
    double    rate;
    double    lenSecs;
} libsox_(AudioData);

static libsox_(AudioData) libsox_(read_audio_file)(const char *file_name)
{
  // Create sox objects and read into int32_t buffer
  sox_format_t *fd;
  fd = sox_open_read(file_name, NULL, NULL, NULL);
  if (fd == NULL)
    abort_("[read_audio_file] Failure to read file");
  
  int nchannels = fd->signal.channels;
  long buffer_size = fd->signal.length;
  double rate = fd->signal.rate;
  int32_t *buffer = (int32_t *)malloc(sizeof(int32_t) * buffer_size);
  size_t samples_read = sox_read(fd, buffer, buffer_size);
  if (samples_read == 0)
    abort_("[read_audio_file] Empty file or read failed in sox_read");
  // alloc tensor 
  THTensor *tensor = THTensor_(newWithSize2d)(nchannels, samples_read / nchannels );
  tensor = THTensor_(newContiguous)(tensor);
  real *tensor_data = THTensor_(data)(tensor);
  // convert audio to dest tensor 
  int x,k;
  for (k=0; k<nchannels; k++) {
    for (x=0; x<samples_read/nchannels; x++) {
      *tensor_data++ = (real)buffer[x*nchannels+k];
    }
  }
  // free buffer and sox structures
  sox_close(fd);
  free(buffer);
  THTensor_(free)(tensor);

  libsox_(AudioData) ret;
  ret.t = tensor;
  ret.nChannels = nchannels;
  ret.bufferSize = buffer_size;
  ret.rate = rate;
  ret.lenSecs = (samples_read / nchannels) / rate;
  return ret;
}

static int libsox_(Main_load)(lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  libsox_(AudioData) data = libsox_(read_audio_file)(filename);
  luaT_pushudata(L, data.t, torch_Tensor);
  return 1;
}

inline void libsox_(TableNum)(lua_State *L, char *key, lua_Number val)
{
   lua_pushstring(L, key);
   lua_pushnumber(L, val);
   lua_settable(L, -3);
}

static int libsox_(Main_load_full)(lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  libsox_(AudioData) data = libsox_(read_audio_file)(filename);  

  lua_newtable(L);

  lua_pushstring(L, "tensor");
  luaT_pushudata(L, data.t, torch_Tensor);
  lua_settable(L, -3);

  libsox_(TableNum)(L, "numChannels", data.nChannels);
  libsox_(TableNum)(L, "bufferSize", data.bufferSize);
  libsox_(TableNum)(L, "rate", data.rate);
  libsox_(TableNum)(L, "lenSecs", data.lenSecs);

  return 1;
}

static const luaL_Reg libsox_(Main__)[] =
{
  {"load", libsox_(Main_load)},
  {"load_full", libsox_(Main_load_full)},
  {NULL, NULL}
};

DLL_EXPORT int libsox_(Main_init)(lua_State *L)
{
  luaT_pushmetatable(L, torch_Tensor);
  luaT_registeratname(L, libsox_(Main__), "libsox");
  // Initialize sox library
  sox_format_init();
  return 1;
}

#endif

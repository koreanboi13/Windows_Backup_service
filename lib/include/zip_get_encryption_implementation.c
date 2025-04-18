/*
  zip_get_encryption_implementation.c -- get encryption implementation
  Copyright (C) 2009-2021 Dieter Baron and Thomas Klausner

  This file is part of libzip, a library to manipulate ZIP archives.
  The authors can be contacted at <info@libzip.org>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "zipint.h"


zip_encryption_implementation
_zip_get_encryption_implementation(zip_uint16_t em, int operation) {
    switch (em) {
    case ZIP_EM_TRAD_PKWARE:
        return operation == ZIP_CODEC_DECODE ? zip_source_pkware_decode : zip_source_pkware_encode;

#if defined(HAVE_CRYPTO)
    case ZIP_EM_AES_128:
    case ZIP_EM_AES_192:
    case ZIP_EM_AES_256:
        return operation == ZIP_CODEC_DECODE ? zip_source_winzip_aes_decode : zip_source_winzip_aes_encode;
#endif

    default:
        return NULL;
    }
}

ZIP_EXTERN int
zip_encryption_method_supported(zip_uint16_t method, int encode) {
    if (method == ZIP_EM_NONE) {
        return 1;
    }
    return _zip_get_encryption_implementation(method, encode ? ZIP_CODEC_ENCODE : ZIP_CODEC_DECODE) != NULL;
}

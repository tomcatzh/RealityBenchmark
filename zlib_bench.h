#ifndef __ZLIB_BENCH_H
#define __ZLIB_BENCH_H

CONTENTS* deflateContent(const CONTENTS* plain, const int level);
CONTENTS* deflateContentBestSpeed(const CONTENTS* plain);
CONTENTS* deflateContentBestCompression(const CONTENTS* plain);
CONTENTS* deflateContentDefaultCompression(const CONTENTS* plain);

CONTENTS* inflateContent(const CONTENTS* compressed);

#endif
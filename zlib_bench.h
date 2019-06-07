#ifndef __ZLIB_BENCH_H
#define __ZLIB_BENCH_H

CONTENTS* deflate_content(const CONTENTS* plain, const int level);
CONTENTS* deflate_content_best_speed(const CONTENTS* plain);
CONTENTS* deflate_content_best_compression(const CONTENTS* plain);
CONTENTS* deflate_content_default_compression(const CONTENTS* plain);

CONTENTS* inflate_content(const CONTENTS* compressed);

#endif
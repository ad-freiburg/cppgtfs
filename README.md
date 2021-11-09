# cppgtfs

Parses GTFS feeds into memory. Extensive validation is applied. Currently used by LOOM, pfaedle, and some student projects. Not perfect, but does the job.

## Usage

```
#include "ad/cppgtfs/Parser.h"

[...]

ad::cppgtfs::Parser parser;
ad::cppgtfs::gtfs::Feed feed;

parser.parse(&feed, "path/to/gtfs/folder");
```

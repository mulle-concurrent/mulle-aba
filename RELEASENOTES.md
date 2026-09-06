### 3.1.6





* world/entry cleanup during storage teardown now uses non-atomic writes,
  avoiding unnecessary barriers in single-threaded paths
* debug and trace output now uses relaxed atomic reads where full ordering
  is not required




* corrected filenames in header comments across all source files
* added missing license header to src/generic/include.h
* updated copyright year from 2015 to 2018 across source headers
* moved API TOC from asset/dox/TOC.md to asset/dox/api/toc/index.md

### 3.1.5

Various small improvements

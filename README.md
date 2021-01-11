# sub-mover

Ever get tired of downloading subtitles and the filenames doesn't match? Suffer no more, sub-mover to the rescue!

## Features

* Auto-matches video-files and subtitles with only the folders as input
* Match single files

## Install

```console
$ git clone https://github.com/ElitCoder/sub-mover.git && cd sub-mover
$ meson builddir && cd builddir
$ meson compile
$ sudo meson install
```

Depending on distribution, the default path '/usr/local/bin' might not be present on PATH.

## Running

sub-mover basically has one use case - mapping stuff together.
```console
$ sub-mover <folder containing subtitles> <folder containing video-files>
```

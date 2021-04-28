# Fresh Hay

> The main part of a rabbit's diet should be unlimited amounts of fresh hay

![Fresh Hay](https://raw.githubusercontent.com/devgru/FreshHay/master/app/icon.jpg)

Fresh Hay is a homebrew app designed to be part of [Kefir](https://github.com/rashevskyv/kefir/).

It downloads firmware version supported by provided Atmosphere/SX OS versions to be installed via ChoiDujourNX/DayBreak.

Fresh Hay relies on config file located at `/switch/FreshHay/target.json`

There's no known problems with applet mode, Fresh Hay does not use that much RAM.

## Building app

App is built using libnx v3. If you don't have libs and toolchain installed just use dockerized runner:

`docker-compose up make`

or set `SWITCH_IP` env variable and send your payload instantly via hbmenu

`docker-compose up makeAndSend`

## TODO

Current version is good enough for its purpose, it is used since early 2020.

Possible upgrades:
- find out how to use `path` param of `nxlink`;
- check that SD card has enough free space to download and unzip archive;
- print remaining waiting time if Mega.nz limit is reached;
- suggest removing archive and corresponding firmware folder if already installed;
- download firmware list and allow choosing specific version;
- replace console with GUI.

## Special Thanks

Icon by [Eucalyp](https://www.flaticon.com/authors/eucalyp) from [FlatIcon.com](https://FlatIcon.com)

[ITotalJustice](https://github.com/ITotalJustice) for the Atmosphere Updater

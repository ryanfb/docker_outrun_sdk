# Docker Out Run SDK

This is a dockerized environment for running/using the [Out Run Arcade SDK](http://www.aaldert.com/outrun/sdk.html), since it was originally developed for Windows. It uses [my `m68k_gcc` Docker image](https://hub.docker.com/r/ryanfb/m68k_gcc/) for the cross-compiler toolchain, then builds the SDK from scratch inside that environment.

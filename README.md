# ahc

> UNDER DEVELOPMENT!

alienhook for Counter-Strike 1.6

Works with most other Half-Life based games.

Written in ANSI C (C89).

## Features

* Fast compilation and loading
* Bunnyhop
* Fast stop
* Multi-directional autostrafer
* Aimbot
* Chams
* No flash
* No aim punch (commented out)
* Thirdperson (commented out)

## TODO

* JSON support for configs
* Proper aimbot hitbox support
* Aimbot visible check
* Expand the Half-Life SDK

## Installing dependencies

### Arch (Artix, Manjaro, etc.)

```
sudo pacman -S git make gcc lib32-gcc-libs mesa
```

### Debian (Ubuntu, etc.)

```
sudo apt install git make gcc-multilib mesa-common-dev
```

### Fedora

```
sudo dnf install glibc-devel.i686 mesa-libGL-devel.i686
```

## Downloading

```
git clone --depth 1 https://git.debooger.xyz/debooger/ahc.git
```

## Building

```
cd ahc
make
```

## Loading

> If the cheat is already loaded it will get reloaded automatically.

```
sudo ./load.sh
```

## Unloading

```
sudo ./unload.sh
```

## Updating

```
git pull
make
```

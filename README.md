# Catherine Full Body HD Patch

This patch provides two rendering modes for Catherine Full Body on the PlaySation Vita and PlaySation TV:

1. 1280x720
2. 960x544 MSAA 4x

Preview images (1280x720): [1](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview1.png?h=assets) [2](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview2.png?h=assets) [3](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview3.png?h=assets) [4](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview4.png?h=assets)

## Installation

1. Install Catherine Full Body (PCSG01179) and update to 1.03.
2. Optionally install [froid_san's English patch](https://forum.devchroma.nl/index.php/topic,133.0.html).
3. Install `catherinefbhd.suprx` under `*PCSG01179` of your taiHEN config.

```
*PCSG01179
ur0:/tai/catherinefbhd.suprx
```

If you are using the 1280x720 patch, install [Sharpscale](https://forum.devchroma.nl/index.php/topic,112.0.html) and turn on "Unlock framebuffer size" in the configuration app.

If you are using a Vita, you can use [udcd-uvc](https://github.com/xerpi/vita-udcd-uvc) to output 1280x720 by USB.

## Performance

Overclocking is required for good performance. For 1280x720, the framerate is between 20-30 FPS with 25-30 FPS in gameplay. For 960x544 MSAA 4x, the framerate is 30 FPS. MSAA does not work in all scenes due to the rendering design of the game.

Video analysis from InquisitionImplied: <https://www.youtube.com/watch?v=uzYS9XtIHiE>

## Building

Dependencies:

- [DolceSDK](https://forum.devchroma.nl/index.php/topic,129.0.html)
- [fnblit and bit2sfn](https://git.shotatoshounenwachigau.moe/vita/fnblit)
- [psp2dbg](https://git.shotatoshounenwachigau.moe/vita/psp2dbg)
- [Terminus font](http://terminus-font.sourceforge.net)
- [taiHEN](https://git.shotatoshounenwachigau.moe/vita/taihen)

Define `PATCH_MODE` when building:

- `PATCH_720` 1280x720
- `PATCH_544_MSAA_4X` 960x544 MSAA 4x

Logging can be configured with CMake variables.

To build dependencies and module:

```sh
cmake .
make dep-all
make
```

## Contributing

Use [git-format-patch](https://www.git-scm.com/docs/git-format-patch) or [git-request-pull](https://www.git-scm.com/docs/git-request-pull) and email me at <asakurareiko@protonmail.ch>.

## Credits

- Testing and video analysis: [InquisitionImplied](https://twitter.com/Yoyogames28)
- 3D buffer offsets: [VitaGrafix patchlist](https://github.com/Electry/VitaGrafixPatchlist)
- Author: 浅倉麗子

## See also

- [Discussion](https://forum.devchroma.nl/index.php/topic,154.0.html)
- [Source code](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd)
- [Bug tracker](https://github.com/cuevavirus/hdpatch/issues)

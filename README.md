# Catherine Full Body HD Patch

[Download binary](https://forum.devchroma.nl/index.php/topic,154.0.html) | [Report bugs](https://github.com/cuevavirus/hdpatch/issues) | [Source code](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/)

This patch changes the 3D render and framebuffer resolutions of Catherine Full Body on the Vita and PSTV to 1280x720. 1280x720 can be output to HDMI or USB ([udcd-uvc](https://github.com/xerpi/vita-udcd-uvc)), or Vita users can enjoy a supersampled image directly on the screen.

Preview images: [1](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview1.png?h=assets) [2](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview2.png?h=assets) [3](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview3.png?h=assets) [4](https://git.shotatoshounenwachigau.moe/vita/catherinefbhd/plain/preview4.png?h=assets)

## Installation

1. Install Catherine Full Body (PCSG01179) and update to 1.03.
2. Optionally install [froid_san's English patch](https://forum.devchroma.nl/index.php/topic,133.0.html).
3. Install the latest version of [Sharpscale](https://forum.devchroma.nl/index.php/topic,112.0.html) and the configuration app.
4. Turn on "Enable Full HD" in the Sharpscale configuration app.
5. Install `catherinefbhd.suprx` under `*PCSG01179` of your taiHEN config.

```
*PCSG01179
ur0:/tai/catherinefbhd.suprx
```

## Performance

Overclocking is required for good performance. Framerate is between 20-30 FPS with 25-30 FPS in gameplay.

Video analysis from InquisitionImplied: <https://www.youtube.com/watch?v=uzYS9XtIHiE>

## Building

Dependencies:

- [DolceSDK](https://forum.devchroma.nl/index.php/topic,129.0.html)
- [fnblit and bit2sfn](https://git.shotatoshounenwachigau.moe/vita/fnblit)
- [Terminus font](http://terminus-font.sourceforge.net)
- [taiHEN](https://git.shotatoshounenwachigau.moe/vita/taihen)

## Contributing

Use [git-format-patch](https://www.git-scm.com/docs/git-format-patch) or [git-request-pull](https://www.git-scm.com/docs/git-request-pull) and email me at <asakurareiko@protonmail.ch>.

## Credits

- Testing and video analysis: [InquisitionImplied](https://twitter.com/Yoyogames28)
- 3D buffer offsets: [VitaGrafix patchlist](https://github.com/Electry/VitaGrafixPatchlist)
- Author: 浅倉麗子

## See more

CBPS ([forum](https://forum.devchroma.nl/index.php), [discord](https://discordapp.com/invite/2ccAkg3))

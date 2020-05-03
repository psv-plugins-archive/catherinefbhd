# Catherine Full Body 720p Patch for PSTV

[Download binary](https://forum.devchroma.nl/index.php/topic,154.0.html) | [Source code](https://git.shotatoshounenwachigau.moe/vita/catherinefb720p/)

This patch changes the render and framebuffer resolutions of Catherine Full Body to 1280x720 on the PSTV.

## Installation

1. Install Catherine Full Body (PCSG01179) and update to 1.03.
2. Optionally install [froid_san's English patch](https://forum.devchroma.nl/index.php/topic,133.0.html).
3. Install the latest version of [Sharpscale](https://forum.devchroma.nl/index.php/topic,112.0.html) and the configuration app.
4. Turn on "Enable Full HD" in the Sharpscale configuration app.
5. Install `catherinefb720.suprx` under `*PCSG01179` of your taiHEN config.

```
*PCSG01179
ur0:/tai/catherinefb720.suprx
```

## Performance

Overclocking is required for good performance. Framerate ranges from 20-30 FPS with 25-30 FPS in gameplay.

Video analysis from [InquisitionImplied](https://twitter.com/Yoyogames28): <https://www.youtube.com/watch?v=uzYS9XtIHiE>

## Building

DolceSDK and a [patched taihen.h](https://github.com/yifanlu/taiHEN/pull/87) are required for building.

## Contributing

Use [git-format-patch](https://www.git-scm.com/docs/git-format-patch) or [git-request-pull](https://www.git-scm.com/docs/git-request-pull) and email me at <asakurareiko@protonmail.ch>.

## Credits

- Testing and video analysis: InquisitionImplied
- Author: 浅倉麗子

## See more

CBPS ([forum](https://forum.devchroma.nl/index.php), [discord](https://discordapp.com/invite/2ccAkg3))

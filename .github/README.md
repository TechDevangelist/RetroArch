# RetroArch 1.10.3 (dingux build) for miyoomini

Based on the dingux version of RetroArch, with video/audio/input driver customized for miyoomini
Specific differences from the dingux version are described below

## Video, Scaling

Video driver is specialized for miyoomini, mainly uses hardware 2D graphics engine (MI_GFX)

Following 4 types of scaling are available depending on Settings > Video > Scaling:

Integer ON  + Aspect ON  ... Integer Scaling
Integer ON  + Aspect OFF ... Integer Scaling + Stretch either width or height to 4:3 aspect ratio, for CRT console emulators
Integer OFF + Aspect ON  ... Aspect Scaling
Integer OFF + Aspect OFF ... Fullscreen

Aspect/Fullscreen scaler method follows Settings > Video > Image Interpolation:

Bicubic		 : Software pre-upscaler + Hardware scaler (reduce blur)
Bilinear	 : Hardware scaler only (blur but slightly faster)
Nearest Neighbor : Software nearest neighbor scaler

NOTE: If Integer:ON and the frame is larger than 640x480, it is automatically turned OFF

Recommended settings:
- Integer ON	   : If you do not want to filter as much as possible even if the screen size is small
- Integer OFF	   : If you want to make the screen size as large as possible
- Aspect  ON	   : Recommended for LCD handheld emulators assuming perfect square pixels
- Aspect  OFF	   : Recommended for CRT console emulators assuming 4:3 screen
- Bicubic	   : Recommended in most cases
- Bilinear	   : If you want to increase the performance as much as possible even if the screen is blurry
- Nearest Neighbor : If you want to make pixels crisp anyway even if the size of each pixels are distorted

Settings > Video > Synchronization > VSync settings setting is activated from 220427 version
Turn ON if the audio driver is sdl/oss and judder is a concern

Settings > Video > Output > Video Rotation is activated from 220430 version
To display this option, Settings > User Interface > Show Advanced Settings must be turned on

Allows VideoFilter to be added later, Normal3x / LCD2x / LCD3x are included as samples (sources are in gfx/video_filters)
LCD filters should always be used with Integer:ON Aspect:ON on emulators
 up to 320pixels width for 2x (for gba,ws,etc.) and up to 213pixels width for 3x (for gb,gg,ngp,etc.)

## Audio

There are three drivers: audioio, oss and sdl

audioio:Directly controls audio hardware of miyoomini(MI_AO)
	Latency can be adjusted in 1ms increments, stabilizing FPS and reducing judder
	However this driver cannot be used with OFW 220419's audioserver (need to turn OFF the AudioFix setting)

oss:	Modified OSS driver exclusively for miyoomini
	Latency is not adjustable due to the miyoomini oss spec, and the default is very high latency of about 340ms
	However, using audioserver with latency_reduction(described below), the latency becomes almost imperceptible

sdl:	customSDL is used when AudioFix:OFF, stockSDL is used when AudioFix:ON
	latancy changes from a minimum of 21 ms in increments of about 10 ms
	Timing is a little rough, that makes judder a bit but slightly faster
	Recommended to turn ON the VSync setting for Video (reduces judder)

The latency default is 64 ms, a comfortable value with all drivers

audioserver, a new feature of OFW 220419, is supported from 220423 version
To play sound with AudioFix:ON, set audio driver to oss or sdl and latancy to at least 96ms in case of sdl

However, due to the audioserver spec, the actual latancy is 340ms(fixed) + setting value, which is quite laggy,
and it also increases judder when scrolling, but a solution to this latency problem is available now (latency_reduction.zip)
With this, the latency setting can be lower than 96ms, read the readme in the zip for instructions on how to use

## Input, Rumble

MENU is assigned to L3 and POWER(actually sleeps so normally unusable) to R3

Miyoomini cannot adjust the strength of the vibration,
so designed to adjust the time to vibrate after the rumble order

The vibration time from the rumble order can be adjusted with the following settings:
Settings > Input > Haptic Feedback/Vibration > Vibration Strength
 100% = 200ms, 90% = 180ms, ..., 10% = 20ms, 5% = 10ms, 0% = no vibration

Vibration strength varies greatly depending on the production lot of the miyoomini,
for devices that vibrate to the maximum, a setting of about 20% seems optimal

## Source

This source is the diff from this version: https://github.com/libretro/RetroArch/releases/tag/v1.10.3
Makefile is based on Makefile.miyoo/dingux
MI libraries and headres are in SYSROOT/usr/lib , SYSROOT/usr/include/sdkdir

# SpriteViewer

## What is it?
SpriteViewer is a simple preview tool for displaying sprites and spritemaps.

## What can it do?
It can show a single sprite tiled in the x and y direction, or as
an animation made up of several sprites in a tileset/spritemap image. When the file changes
on disk, the displayed sprite is updated. You can also adjust the scaling and animation speed.

## What would I use it for?
Running SpriteViewer in a separate window lets you get a realtime preview
of how your sprites tile and whether they are truly seamless, while you draw
and design your sprites and animations in a separate editor like GIMP or Photoshop.
A lot of image editing software lacks support for tiled preview and sprite animations.
The idea behind TileViewer is to address this and to help with your workflow.

## How do I start it? What options are available?
Starting SpriteViewer without any options shows the usage:

<pre><code>
Usage:  [options] file
Options:
  -w <width>			Sprite width
  -h <height>			Sprite height
  -s <scale>			Sprite scale
  -f <fps>				Animation frames per second
  -i start:end			The start- and end sprite indices.
  -t					Enable tiling/repeat
</code></pre>

Example #1: Given a spritemap with 32x32 sprites, show the first frame tiled in the X and Y directions.

spriteview -w 32 -h 32 -t -i 0:0 ~/Pictures/spritemap.png

Example #2: Given a spritemap with 16x16 sprites, show an animation using the sprites from index 7 to (including) index 14 without tiling, scaled up 4x and with 8 frames per second.

spriteview -w 16 -h 16 -s 4 -f 8 -i 7:15 ~/Pictures/spritemap.png

Example #3 Same as example #2, but with tiling and without scaling.

spriteview -t -w 16 -h 16 -f 8 -i 7:15 ~/Pictures/spritemap.png


# DeltaInject
A proof of concept app, that injects into Deltarune's RAM, and replaces dialogs. 
Tested on Chapter 4, Windows 10, Deltarune 17.
Reset `minPageOffset` to 0x0, find the first page offset ()

# Information about pages
I don't know how they work. 

They are 16MB, Private Memory, have infinite amount of blocks, usually:
```
16MB
---
8MB
---
7MB
---
1MB
---
```
Or, something similar. 

This app has a way to auto-find the page, set minPageOffset to 0x0 for it to do that. If you know the first address of the page (first address of first block of the page), then you can set it into `minPageOffset` for faster processing.

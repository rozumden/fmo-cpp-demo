# FMO detection (demo version)


## Prerequisites
```
opencv 
boost
```

## To compile
```
mkdir build
cd build
cmake ..
make
sudo make install
```

## To run real-time on a camera 
Integer after `--camera` for camera index.

> fmo-desktop --camera 0 --utiademo

While running: space to stop, enter to make a step, "n" to enter name to score table, "1" to hide score table, "2" to show score table, "r" to start or stop recording, "h" for help subwindow, "a" and "m" to switch between automatic and manual mode, esc to exit.

> fmo-desktop --camera 0 --demo

> fmo-desktop --camera 0 --tutdemo

While running: different modes switched by "0", "1", "2", "3", "4".

> fmo-desktop --camera 0 --removal

> fmo-desktop --camera 0 --debug

While running: "d" for difference image, "i" for thresholded, "t" for distance transform, "o" for original image, etc.
 
  
## Parameters

`--exposure <float> `

Set exposure value. Should be between 0 and 1. Usually between 0.03 and 0.1.

`--fps <int>`

Set number of frames per second.

`--p2cm <float>`

Set how many cm are in one pixel on object. Used for speedm estimation. More dominant than --radius.
            
`--dfactor <float>`

Differential image threshold factor. Default 1.0.

`--radius  <float> `

Set object radius in cm. Used for speed estimation. Used if --p2cm is not specified. By default used for tennis/floorball: 3.6 cm.


## To run for a specific video:
> fmo-desktop --input <path> --demo
  
## Help
> fmo-desktop --help

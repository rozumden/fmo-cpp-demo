# fmo-cpp
FMO detection (demo version)

Prerequisites:
1. opencv 
2. boost

To compile:
1. mkdir build
2. cd build
3. cmake ..
4. make
5. sudo make install

To run real-time on a camera (integer for camera index):
1. fmo-desktop --camera 0 --utiademo
2. fmo-desktop --camera 0 --demo
3. fmo-desktop --camera 0 --tutdemo
4. fmo-desktop --camera 0 --removal
5. fmo-desktop --camera 0 --debug

To run for a specific video:
fmo-desktop --input <path> --demo
  
Parameters: 
--exposure  Set exposure value. Should be between 0 and 1. Usually between 0.03 and 0.1.
--fps       Set number of frames per second.
--p2cm      Set how many cm are in one pixel on object. Used for speed
            estimation. More dominant than --radius.
--dfactor   Differential image threshold factor. Default 1.0.
--radius    Set object radius in cm. Used for speed estimation. Used if --p2cm
            is not specified. By default used for tennis/floorball: 3.6 cm.


Help:
fmo-desktop --help

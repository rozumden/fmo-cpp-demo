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
(while running: space to stop, enter to make a step, "n" to enter name to score table, "1" to hide score table, "2" to show score table, "r" to start or stop recording, "h" for help subwindow, "a" and "m" to switch between automatic and manual mode, , esc to exit)
2. fmo-desktop --camera 0 --demo
3. fmo-desktop --camera 0 --tutdemo
(while running: different modes switched by "0", "1", "2", "3", "4")
4. fmo-desktop --camera 0 --removal
5. fmo-desktop --camera 0 --debug
(while running: "d" for difference image, "i" for thresholded, "t" for distance transform, "o" for original image, etc.)
 
  
Parameters: 

--exposure  Set exposure value. Should be between 0 and 1. Usually between 0.03 and 0.1.

--fps       Set number of frames per second.

--p2cm      Set how many cm are in one pixel on object. Used for speedm estimation. More dominant than --radius.
            
--dfactor   Differential image threshold factor. Default 1.0.

--radius    Set object radius in cm. Used for speed estimation. Used if --p2cm is not specified. By default used for tennis/floorball: 3.6 cm.


To run for a specific video:
fmo-desktop --input <path> --demo
  
Help:
fmo-desktop --help

Sample programs to test if there is a speed difference between using threads or not when retrieving frames from several cameras for stereovision.

The program [grab then retrieve](grab then retrieve) simply use the OpenCV function `grab` on the two webcam, one after another, and then `retrieve`, one after another:

```
myGrab(1, captureCam1);  // grab a frame from camera 1
myGrab(2, captureCam2);  // grab a frame from camera 2

myRetrieve(1, captureCam1, frameCam1);  // retrieve a frame from camera 1
myRetrieve(2, captureCam2, frameCam2);  // retrieve a frame from camera 2
```

The program [threads](threads) use a main thread, and two special threads (one for each webcam) to grab and retrieve the pictures.

Those programs do nothing else aside grabbing and retrieving frames.

With the non-threaded program, I was able to get 20 FPS (20 frames grabbed per second, for each camera). I thought that the threaded program would improve the FPS, but I got the same result: 20 FPS. However, I think that adding processing to these programs will make a difference appear (at least if you have a multicore computer): the framerate of the non-threaded program would be affected, whereas as the processing would occur in a different thread in the threaded program, the framerate should not been affected (if there is not too much processing).


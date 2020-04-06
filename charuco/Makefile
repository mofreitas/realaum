.SUFFIXES:
.SUFFIXES: .c .cpp

CC = gcc
GCC = g++

OPENCV_CPP_PATH = /usr/include/include/opencv4

OPENCV_LIBS = -L/usr/include/lib /usr/include/lib/libopencv_dnn.so -lopencv_dnn /usr/include/lib/libopencv_ml.so -lopencv_ml /usr/include/lib/libopencv_objdetect.so -lopencv_objdetect /usr/include/lib/libopencv_stitching.so -lopencv_stitching /usr/include/lib/libopencv_calib3d.so -lopencv_calib3d /usr/include/lib/libopencv_features2d.so -lopencv_features2d /usr/include/lib/libopencv_highgui.so -lopencv_highgui /usr/include/lib/libopencv_videoio.so -lopencv_videoio /usr/include/lib/libopencv_imgcodecs.so -lopencv_imgcodecs /usr/include/lib/libopencv_video.so -lopencv_video /usr/include/lib/libopencv_photo.so -lopencv_photo /usr/include/lib/libopencv_imgproc.so -lopencv_imgproc /usr/include/lib/libopencv_flann.so -lopencv_flann /usr/include/lib/libopencv_core.so -lopencv_core /usr/include/lib/libopencv_aruco.so -lopencv_aruco

#Libs.private: -ldl -lm -lpthread -lrt

.c:
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(GL_LIBS) -o $@

.cpp:
	$(GCC) -std=c++11 -O2 -I $(OPENCV_CPP_PATH) $< -o $@ $(OPENCV_LIBS) `pkg-config --cflags --libs gstreamer-1.0`
	



.SUFFIXES:
.SUFFIXES: .c .cpp

CC = gcc
GCC = g++

OPENCV_CPP_PATH = /usr/include/include/opencv4
GLAD_PATH = ~/Documentos/TCC/realaum/include

LIBS_PATH = /usr/include/lib

OPENCV_LIBS = -L$(LIBS_PATH) $(LIBS_PATH)/libopencv_dnn.so $(LIBS_PATH)/libopencv_ml.so -lopencv_ml $(LIBS_PATH)/libopencv_objdetect.so -lopencv_objdetect $(LIBS_PATH)/libopencv_stitching.so -lopencv_stitching $(LIBS_PATH)/libopencv_calib3d.so -lopencv_calib3d $(LIBS_PATH)/libopencv_features2d.so -lopencv_features2d $(LIBS_PATH)/libopencv_highgui.so -lopencv_highgui $(LIBS_PATH)/libopencv_videoio.so -lopencv_videoio $(LIBS_PATH)/libopencv_imgcodecs.so -lopencv_imgcodecs $(LIBS_PATH)/libopencv_video.so -lopencv_video $(LIBS_PATH)/libopencv_photo.so -lopencv_photo $(LIBS_PATH)/libopencv_imgproc.so -lopencv_imgproc $(LIBS_PATH)/libopencv_flann.so -lopencv_flann $(LIBS_PATH)/libopencv_core.so -lopencv_core $(LIBS_PATH)/libopencv_aruco.so -lopencv_aruco -lGL -ldl -lglfw -lassimp
.c:
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(GL_LIBS) -o $@

.cpp:
	$(GCC) -std=c++11 -O2 -I $(GLAD_PATH) -I $(OPENCV_CPP_PATH) glad.c $< -o $@.out $(OPENCV_LIBS) -fopenmp -pthread `pkg-config --cflags --libs gstreamer-1.0`
	
#g++ -std=c++11 -O2 -I /opt/include/include/opencv4 webstream.cpp -o webstream -L/opt/include/lib /opt/include/lib/libopencv_ml.so -lopencv_ml /opt/include/lib/libopencv_objdetect.so -lopencv_objdetect /opt/include/lib/libopencv_stitching.so -lopencv_stitching /opt/include/lib/libopencv_calib3d.so -lopencv_calib3d /opt/include/lib/libopencv_features2d.so -lopencv_features2d /opt/include/lib/libopencv_highgui.so -lopencv_highgui /opt/include/lib/libopencv_videoio.so -lopencv_videoio /opt/include/lib/libopencv_imgcodecs.so -lopencv_imgcodecs /opt/include/lib/libopencv_video.so -lopencv_video /opt/include/lib/libopencv_photo.so -lopencv_photo /opt/include/lib/libopencv_imgproc.so -lopencv_imgproc /opt/include/lib/libopencv_flann.so -lopencv_flann /opt/include/lib/libopencv_core.so -lopencv_core



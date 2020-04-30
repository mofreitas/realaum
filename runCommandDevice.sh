#!/bin/bash

#ssh pi@192.168.0.21 'gst-launch-1.0 -vvv videotestsrc ! videoconvert ! video/x-raw ! videorate ! video/x-raw, framerate=20/1 ! queue ! omxh264enc ! queue ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host=192.168.0.16' > ./output_gst.txt

#ssh pi@192.168.0.21 'nohup gst-launch-1.0 -vvv videotestsrc ! videoconvert ! video/x-raw ! videorate ! video/x-raw, framerate=20/1 ! queue ! omxh264enc ! queue ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host=192.168.0.16 2>&1 > ./output_gst.txt & echo $!'

ssh pi@192.168.0.21 -t 'nohup gst-launch-1.0 -vvv videotestsrc ! videoconvert ! video/x-raw ! videorate ! video/x-raw, framerate=20/1 ! queue ! omxh264enc ! queue ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host=192.168.0.16 2>&1 > ./output_gst.txt' 

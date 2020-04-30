> Instalar o GLFW com o comando `sudo apt install libglfw3`

1. Gera tabuleiro com `/charuco/chessboardGenerator`
2. Calibra camera com o `/charuco/calib` usando o comando `./calib "../cameraParameters.txt" -h=7 -w=5 -sl=0.07 -ml=0.05 -d=2` com:
    1. h=número de quadros na vertical
    2. w=número de quadros na horizontal
    3. sl=tamanho do quadrado em metros
    4. ml=tamanho do marcador em metros
    5. d=dicionario utilizado para gerar tabuleiro
3. Compila o oficial.cpp e roda utilizando `./oficial -d` para testar usando a camera do proprio computador.

4. Gera a chave no computador usando o comando `ssh-keygen`
5. Copia a chave para raspiberry usando o comando `ssh-copy-id -i ~/.ssh/mykey user@host`
6. Instalar no rasp:

```
sudo apt-get install gstreamer1.0-tools
sudo apt-get install gstreamer1.0-plugins-base
sudo apt-get install gstreamer1.0-plugins-good
sudo apt-get install gstreamer1.0-omx
```
7. Pipeline do raspberry (caso necessário para debug): gst-launch-1.0 -vvv v4l2src device=/dev/video0 ! videoconvert ! video/x-raw, width=640 ! videorate ! video/x-raw, framerate=20/1 ! queue ! omxh264enc ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host=[IP do computador]
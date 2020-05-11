1. 
Instalar o GLFW com o comando `sudo apt install libglfw3-dev`
Instalar o GLM com o comando `sudo apt install libglm-dev`
Instalar o ASSIMP com o comando `sudo apt install libassimp-dev`


2. Gera tabuleiro com `/charuco/chessboardGenerator`
3. Calibra camera com o `/charuco/calib` usando o comando `./calib "../cameraParameters.txt" -h=7 -w=5 -sl=0.07 -ml=0.05 -d=2` com:
    1. Arquivo com os parâmetros de calibração
    2. h=número de quadros na vertical
    3. w=número de quadros na horizontal
    4. sl=tamanho do quadrado em metros
    5. ml=tamanho do marcador em metros
    6. d=dicionario utilizado para gerar tabuleiro

4. Gera a chave no computador usando o comando `ssh-keygen`
5. Copia a chave para raspberry usando o comando `ssh-copy-id -i ~/.ssh/mykey user@ip_rasp`
6. Instalar no rasp:
```
sudo apt-get install gstreamer1.0-tools
sudo apt-get install gstreamer1.0-plugins-base
sudo apt-get install gstreamer1.0-plugins-good
sudo apt-get install gstreamer1.0-omx
```

7. Compila o oficial.cpp e roda utilizando 
    - `./oficial -d -c <caminho dos arquivo com parâmetros de calibração>` para testar usando a camera do proprio computador (sem mandar para o dispositivo móvel).
    - `./oficial -c <caminho dos arquivo com parâmetros de calibração>` para usar a câmera do próprio computador porém enviando para o celular as imagens finais
    - `./oficial -c <caminho dos arquivo com parâmetros de calibração> -pi <login_pi>@<ip_pi>` Para utilizar o programa com o raspberry e o dispositivo móvel.

8. Pipeline do raspberry (caso necessário para debug): gst-launch-1.0 -vvv v4l2src device=/dev/video0 ! videoconvert ! video/x-raw, width=640 ! videorate ! video/x-raw, framerate=20/1 ! queue ! omxh264enc ! rtph264pay config-interval=1 ! queue ! udpsink port=5000 host=**ip_computador**

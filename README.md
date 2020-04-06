> Instalar o GLFW com o comando `sudo apt install libglfw3`

1. Gera tabuleiro com `/charuco/chessboardGenerator`
2. Calibra camera com o `/charuco/calib` usando o comando `./calib "./saida2.txt" -h=7 -w=5 -sl=0.07 -ml=0.05 -d=2` com:
    1. h=número de quadros na vertical
    2. w=número de quadros na horizontal
    3. sl=tamanho do quadrado em metros
    4. ml=tamanho do marcador em metros
    5. d=dicionario utilizado para gerar tabuleiro
3. Compila o oficial.cpp e roda utilizando `./oficial -ip <ip> -d` para testar usando a camera do proprio computador.

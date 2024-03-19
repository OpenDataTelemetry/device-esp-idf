# device-esp-idf

## Init docker container in vscode

## Share USB with container
### On Windows:
Download: https://github.com/espressif/esptool/releases
```bash
esp_rfc2217_server -v -p 4000 COM3
```
### On Linux/Mac:
```bash
pip install esptool
# Linux
esp_rfc2217_server.py -v -p 4000 /dev/ttyUSB0
# Mac
esp_rfc2217_server.py -v -p 4000 /dev/tty.usbserial-0001
```

## From terminal:
### Build
Host: 
  * Windows/Mac: host.docker.internal
  * Linux: 172.17.0.1
    ```bash
    docker run --rm -v <host_path>:/workspaces \
        -w /workspaces espressif/idf:release-v4.4 \
        sh -c "idf.py build"
    ```

### Flash:
Host: 
  * Windows/Mac: host.docker.internal
  * Linux: 172.17.0.1
    ```bash
    docker run --rm -v <host_path>:/workspaces \
        -w /workspaces espressif/idf idf.py \
        --port 'rfc2217://HOST:4000?ign_set_control' \
        flash
    ```

### Flash and Monitor :
Host: 
  * Windows/Mac: host.docker.internal
  * Linux: 172.17.0.1
    ```bash
    docker run --rm -v <host_path>:/workspaces \
        -w /workspaces espressif/idf idf.py \
        --port 'rfc2217://HOST:4000?ign_set_control' \
        flash monitor
    ```

## From Container:
SerialPath: 
  * Windows:
  * Mac: /dev//tty.usbserial-0001
  * Linux: /dev/ttyUSB0
  
### Attach to Container:
```bash
docker run -it --device=SERIAL_PATH:/dev/ttyUSB0 \
    -v $PWD:/workspaces -w /workspaces espressif/idf:release-v4.4 \
    idf.py monitor -p SERIAL_PATH
```
### Build
```bash
idf.py build
```
### Flash
```bash
idf.py --port rfc2217://HOST:4000?ign_set_control flash
```
### Flash and Monitor
```bash
idf.py --port rfc2217://HOST:4000?ign_set_control flash monitor
```











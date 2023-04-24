





```bash
docker run -it --device=/dev/ttyUSB0:/dev/ttyUSB0 -v $PWD:/workspaces -w /workspaces espressif/idf:release-v4.4 idf.py monitor -p /dev/ttyUSB0
```

device reports readiness to read but returned no data (device disconnected or multiple access on port?)





idf.py --port rfc2217://host.docker.internal:4000?ign_set_control flash











From host

Flash:

docker run --rm -v ~/Git/OpenDataTelemetry/device-data-esp32-idf/mqtt_client:/workspaces -w /workspaces espressif/idf:release-v4.4 sh -c "idf.py --port rfc2217://172.17.0.1:4000?ign_set_control flash"

Build:

docker run --rm -v ~/Git/OpenDataTelemetry/device-data-esp32-idf/mqtt_client:/workspaces -w /workspaces espressif/idf:release-v4.4 sh -c "idf.py build"



From COntainer:

Build

idf.py build

Flash

idf.py --port rfc2217://172.17.0.1:4000?ign_set_control flash

MOnitor

idf.py --port rfc2217://172.17.0.1:4000?ign_set_control flash

Flash and Monitor

idf.py --port rfc2217://172.17.0.1:4000?ign_set_control flash monitor

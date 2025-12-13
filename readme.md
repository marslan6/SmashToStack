ls /dev/ttyACM* -> Find available devices
python3 create_exploit.py -> create an exploit bin file
python3 sendExploit.py exploit --device /dev/ttyACM0 -> send overflow message to board
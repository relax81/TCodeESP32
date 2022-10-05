@ECHO OFF

SET /P comport=Enter a COM port (default: COM3):
IF NOT DEFINED comport SET "comport=COM3"

.\esptool\esptool.exe --chip esp32 --port %comport% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB --erase-all 0x0 release.bin

pause
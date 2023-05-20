# GeigerOS
This repository contains all versions of the GeigerOS software series, as well as the individual sub-versions for each specific device. There is also a manual for each device.

![GMZ-06](https://github.com/codemarv42/GeigerOS/blob/main/Media/GMZ-06/PXL_20230520_192320395~2.jpg)


![GMZ-05](https://github.com/codemarv42/GeigerOS/blob/main/Media/GMZ-05/4e235e24-2c78-430c-acdd-6adb93e1a7e1.PNG)

## Flash BIN files to the Geiger counter

- Download the flash tool: https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.4.zip
- Unzip the downloaded ZIP file and copy the selected BIN file into the bin folder contained in the unzipped file
- Start the exe file and select esp32
- Press the upper first box and then select the path of the bin file
- write next to the "@": 0x10000
- Then connect the Geiger counter to the pc and select the correct COM port
- Click on start und press the boot button on the Geiger counter

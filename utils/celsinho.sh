#!/bin/bash

# Set up ESP-IDF environment
source /home/doidobr/esp/v5.3.3/esp-idf/export.sh

# Move to project root
cd ..

# Prepare log file
rm -f utils/log.txt
touch utils/log.txt

# Run monitor inside a pseudo-TTY and save output to file with real-time flushing
timeout 10s script -q -f -c "idf.py -p /dev/ttyUSB0 monitor" /dev/stdout | tee utils/log.txt

# Run the Python script using the captured log
python3 utils/gantt.py utils/log.txt

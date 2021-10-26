#!/bin/bash -x
rm -r -f .venv .pio
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install wheel
pip install platformio

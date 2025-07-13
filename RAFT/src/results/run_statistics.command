#!/bin/bash

# Cambia directory alla posizione di questo file (cioè del .command)
cd "$(dirname "$0")"

# Avvia lo script Python
python3 readStatistics.py


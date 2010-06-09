#!/bin/bash

grep -i ERROR test_log.txt | sed "s/.*ERROR: //" | sort | uniq

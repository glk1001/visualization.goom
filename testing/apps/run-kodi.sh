#!/bin/bash

set -u

echo "$@" >/tmp/kodi_args.txt

kodi &


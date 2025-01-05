#!/bin/bash

export LD_LIBRARY_PATH=`realpath Walnut/Walnut-Modules/Walnut-Networking/vendor/GameNetworkingSockets/bin/Linux`
make -j8

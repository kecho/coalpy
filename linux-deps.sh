#!/bin/sh 
apt-get install libopenexr-dev=2.3.0* &&\
apt-get install python3.9 &&\
apt-get install python3.9-dev &&\
apt-get install libsdl2-dev=2.0.10+dfsg1-3

# vulkan lunar SDK
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
wget -qO /etc/apt/sources.list.d/lunarg-vulkan-bionic.list http://packages.lunarg.com/vulkan/lunarg-vulkan-bionic.list
apt update
apt install vulkan-sdk

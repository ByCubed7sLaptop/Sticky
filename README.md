# Sticky 📝📌

Sticky is a lightweight desktop sticky notes application built with C++ and Qt 6, designed primarily for Ubuntu

Features
- create and manage multiple sticky notes simultaneously
- saved as seperate .md files.
- custom save location
- pin notes to stay on top

## Requirements
The following dependencies are required to build Sticky:

- C++ compiler with C++17 support
- Qt 6 Widgets development libraries
- pkg-config

These can be installed using:
```
sudo apt update
sudo apt install build-essential g++ pkg-config qt6-base-dev
```

## Building
Clone the repository:
```
git clone https://github.com/ByCubed7sLaptop/Sticky.git
cd Sticky
```

Run build script:
```
chmod +x build.sh
./build.sh
```
This will compile the executable at: `build/sticky`


## Running
Once the build has completed, launch Sticky using:
```
./build/sticky
```

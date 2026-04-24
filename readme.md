# mowgli_unicore_gnss

ROS 2 C++ driver for Unicore UM982 GNSS receivers.

This repository is now organized as a clean ROS 2 package with the driver implementation in `src/`, public headers in `include/mowgli_unicore_gnss/`, and runtime configuration in `config/um982.yaml`.

## Build

```bash
colcon build --packages-select mowgli_unicore_gnss
```

## Run

```bash
ros2 launch mowgli_unicore_gnss um982_launch.py
```

## Configuration

The default runtime parameters are in `config/um982.yaml`.

## Legacy code

Old C and Python legacy code has been moved to the `legacy/` directory.

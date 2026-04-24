# syntax=docker/dockerfile:1.7
# =============================================================================
# Unicore UM982 GNSS driver (mowgli_unicore_gnss) — ROS 2 C++
#
# Publishes:
#   /gnss/fix           sensor_msgs/NavSatFix (NMEA GGA or Unicore PVTSLNA)
#   /gnss/azimuth       compass_msgs/Azimuth (heading from HDT or HPR)
#   /gnss/diagnostics   diagnostic_msgs/DiagnosticArray (status + counters)
#
# Serial device mounted at runtime: /dev/ttyUSB0 (configurable via um982.yaml)
# =============================================================================

# ─── Builder ────────────────────────────────────────────────────────────────
FROM ros:kilted-ros-base AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://azure.archive.ubuntu.com/ubuntu|g' /etc/apt/sources.list.d/*.sources 2>/dev/null || true

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      git \
      python3-colcon-common-extensions \
      ros-kilted-rclcpp \
      ros-kilted-sensor-msgs \
      ros-kilted-diagnostic-msgs \
      ros-kilted-compass-interfaces

WORKDIR /ws/src
COPY . mowgli_unicore_gnss

WORKDIR /ws
RUN . /opt/ros/kilted/setup.sh \
 && colcon build --merge-install \
      --packages-select mowgli_unicore_gnss \
      --cmake-args -DCMAKE_BUILD_TYPE=Release \
                   -Wno-dev \
 && rm -rf /ws/build /ws/log /ws/src

# ─── Runtime ────────────────────────────────────────────────────────────────
FROM ros:kilted-ros-base

ARG DEBIAN_FRONTEND=noninteractive

RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://azure.archive.ubuntu.com/ubuntu|g' /etc/apt/sources.list.d/*.sources 2>/dev/null || true

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      ros-kilted-rmw-cyclonedds-cpp \
      ros-kilted-compass-interfaces

COPY --from=builder /ws/install /opt/mowgli_unicore_gnss

COPY ros2_entrypoint.sh /ros2_entrypoint.sh
COPY start_um982.sh /start_um982.sh
RUN chmod +x /ros2_entrypoint.sh /start_um982.sh

ENTRYPOINT ["/ros2_entrypoint.sh"]
CMD ["/start_um982.sh"]

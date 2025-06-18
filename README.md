# RouterOSupgrade
# Runs basic upgrade on ip range, checks installed packages and architecture on a RouterOS device.
# Downloads a specific .npk file using /tool fetch
# reboots

# autoUpgrade,autoUpgradeDebug
# g++ -o autoUpgrade autoUpgrade.cpp
# ./ros_auto_upgrade 7.17_ab238 # specify the version to upgrade to.
#
# autoUpgradeDebug with added debug info for development. tested18.06.2025ok

# upgrade:
# upgrade or downgrade MikroTik RouterOS devices in bulk over the network. You tell it the RouterOS version, the IP address range, upgrade or downgrade. 
#
# Pings each device in the IP range to check if it’s online.
#
# SSHs into each device to find out its CPU architecture and which packages are installed.
#
# Downloads the right RouterOS package files directly onto each device.
#
# Runs either a downgrade or reboot command to apply the changes.
#
# Does all this in parallel using multiple threads to speed things up.
#
# It also supports a debug mode to show detailed info during the process.
#
# 

./upgrade <ros_version> <ip_prefix> <start_ip>-<end_ip> [--debug] [-d]

Parameters:

<ros_version> — the RouterOS version you want to install, e.g. 7.21_ab172

<ip_prefix> — the first three parts of the IP address, e.g. 192.168.1

<start_ip>-<end_ip> — the range of last octets of IP addresses to scan, e.g. 199-205

--debug (optional) — show detailed debug info in the console

-d (optional) — run downgrade instead of upgrade

# ./upgrade 7.21_ab176 192.168.1 198-202 //tested18.06.2025ok

Link hidden.
# upgrade

**Bulk upgrade or downgrade of MikroTik RouterOS devices over the network.**

`upgrade` allows you to remotely update or downgrade multiple MikroTik devices at once using their IP range and desired RouterOS version.

---

## Features

- Pings each device in the IP range to check if it’s online.
- SSHs into each device to detect CPU architecture and installed packages.
- Downloads the correct RouterOS package to each device.
- Executes upgrade or downgrade (with reboot) as specified.
- Runs all operations in parallel using multi-threading for speed.
- Debug mode available for detailed logging.

---

## Usage

```bash
Set your repo link for .npk files. 
std::string packageURL = "https://example.eu/routeros/" + rosVersion + "/" + fileName;

g++ upgrade.cpp -o upgrade
./upgrade <ros_version> <ip_prefix> <start_ip>-<end_ip> [--debug] [-d]
./upgrade 7.21_ab175 192.168.20 198-202
```

# GUI version
Created with Qt. 

![image](https://github.com/user-attachments/assets/eaacea71-3e21-40b7-b87f-77182042461d)


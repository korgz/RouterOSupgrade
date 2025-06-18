#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <thread>
#include <mutex>

const std::string USER = "admin";

bool DEBUG = false;      // If true, prints all debug info to console
bool DOWNGRADE = false;  // If true, runs downgrade instead of upgrade

std::mutex io_mutex;     // Used to avoid messy printing when multiple threads print at once

// Runs a shell command and returns its output as a string.
// If DEBUG is on, it will print the command being run.
// Uses popen to grab the output from the command.
std::string runCommand(const std::string& cmd) {
    if (DEBUG) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[DEBUG] CMD: " << cmd << std::endl;
    }
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";  // if command fails, return empty string

    char buffer[256];
    std::string result;

    // Read all output line by line
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

// Takes the output of "/system package print" and extracts just the package names.
// We use a regex to find lines starting with a number, then grab the package name from that line.
std::vector<std::string> extractPackageNames(const std::string& packageOutput) {
    std::vector<std::string> packages;
    std::regex nameRegex("\\s*\\d+\\s+(\\S+)");
    std::smatch match;
    std::istringstream stream(packageOutput);
    std::string line;

    // Check each line, and if it matches our regex, pull out the package name
    while (std::getline(stream, line)) {
        if (std::regex_search(line, match, nameRegex)) {
            packages.push_back(match[1]);  // package name is first captured group
        }
    }
    return packages;
}

// This function handles everything for one device IP:
// ping it, get architecture, list packages, fetch needed files,
// and either downgrade or reboot the device.
void processIP(const std::string& rosVersion, const std::string& ip) {
    {
        // Tell user which IP we are working on
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "\n[*] Scanning " << ip << "..." << std::endl;
    }

    // Ping to check if device is reachable - if not, skip it
    if (std::system(("ping -c 1 -W 1 " + ip + " > /dev/null").c_str()) != 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[-] No response from " << ip << std::endl;
        return;
    }

    // SSH into device and grab the architecture name (like mipsbe or arm64)
    // We grep and awk the output to get just the architecture string
    std::string archCmd = "ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no " + USER + "@" + ip +
                          " \"/system resource print\" 2>/dev/null | grep architecture-name | awk '{print $2}'";
    std::string architecture = runCommand(archCmd);

    // Remove trailing whitespace and newlines from the architecture string
    architecture.erase(architecture.find_last_not_of(" \n\r\t") + 1);

    // If we cant get architecture info, report and skip
    if (architecture.empty()) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[-] Could not read architecture from " << ip << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[*] Architecture: " << architecture << std::endl;
    }

    // SSH again to get the list of installed packages on the device
    std::string pkgCmd = "ssh -o StrictHostKeyChecking=no " + USER + "@" + ip +
                         " \"/system package print without-paging\"";
    std::string packageOutput = runCommand(pkgCmd);

    // Extract the package names from the output we got above
    std::vector<std::string> packageNames = extractPackageNames(packageOutput);

    {
        // Print the packages installed on the device
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[*] Installed packages:" << std::endl;
        for (const auto& pkg : packageNames) {
            std::cout << "    - " << pkg << std::endl;
        }
    }

    // Now, for each package, create the download URL and tell device to fetch it
    for (const auto& pkg : packageNames) {
        std::string fileName = pkg + "-" + rosVersion + "-" + architecture + ".npk";
        std::string packageURL = "https://hidden/routeros/" + rosVersion + "/" + fileName;

        // ssh fetch command to download the package file on the device itself
        std::string fetchCmd = "ssh -o StrictHostKeyChecking=no " + USER + "@" + ip +
                               " \"/tool fetch url=\\\"" + packageURL + "\\\"\"";

        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "[*] Fetching: " << fileName << std::endl;
        }
        // Actually run the fetch command on the device
        std::system(fetchCmd.c_str());
    }

    // After fetching, either downgrade or reboot depending on the flag
    std::string actionCmd;
    if (DOWNGRADE) {
        // Downgrade requires us to answer 'y' to prompt; using ssh with pseudo-tty to send it
        actionCmd = "ssh -tt -o StrictHostKeyChecking=no " + USER + "@" + ip +
                     " \"/system package downgrade\" < <(echo y)";
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "[*] Downgrading " << ip << "..." << std::endl;
        }
    } else {
        // Normal upgrade just reboots the device
        actionCmd = "ssh -o StrictHostKeyChecking=no " + USER + "@" + ip + " \"/system reboot\"";
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "[*] Rebooting " << ip << "..." << std::endl;
        }
    }

    // Run the downgrade or reboot command
    std::system(actionCmd.c_str());

    {
        // Final message to show the operation started successfully
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "[✓] " << (DOWNGRADE ? "Downgrade" : "Upgrade") << " initiated for " << ip << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }
}

// Program entry: handle arguments and run threads for all IPs
int main(int argc, char* argv[]) {
    // If not enough args, print usage and quit
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <ros_version> <ip_prefix> <start-end> [--debug] [-d]\n";
        return 1;
    }

    // Grab the version, IP prefix, and IP range from args
    std::string rosVersion = argv[1];
    std::string ipPrefix = argv[2];
    std::string range = argv[3];

    // Split the IP range into start and end numbers
    size_t dashPos = range.find('-');
    if (dashPos == std::string::npos) {
        std::cerr << "Invalid IP range format. Use start-end.\n";
        return 1;
    }
    int startIP = std::stoi(range.substr(0, dashPos));
    int endIP = std::stoi(range.substr(dashPos + 1));

    // Look for optional flags --debug or -d for downgrade
    for (int i = 4; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            DEBUG = true;
            std::cout << "[DEBUG] Debug mode enabled\n";
        } else if (arg == "-d") {
            DOWNGRADE = true;
            std::cout << "[DEBUG] Downgrade mode enabled\n";
        }
    }

    // Create one thread per IP in the given range to speed up processing
    std::vector<std::thread> threads;
    for (int i = startIP; i <= endIP; ++i) {
        std::ostringstream ipStream;
        ipStream << ipPrefix << "." << i;
        threads.emplace_back(processIP, rosVersion, ipStream.str());
    }

    // Wait for all threads to finish before exiting
    for (auto& t : threads) {
        t.join();
    }

    // All done
    std::cout << "\n[✓] All reachable devices processed.\n";
    return 0;
}

#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <algorithm>

const std::string IP_PREFIX = "192.168.88";
const int START_IP = 2;
const int END_IP = 254;
const std::string USER = "admin";

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ros_version>\n";
        std::cerr << "Example: " << argv[0] << " 7.17_ab238\n";
        return 1;
    }

    std::string rosVersion = argv[1];

    for (int i = START_IP; i <= END_IP; ++i) {
        std::ostringstream ipStream;
        ipStream << IP_PREFIX << "." << i;
        std::string ip = ipStream.str();

        std::cout << "\n[*] Checking device at " << ip << "..." << std::endl;

        // Check if host is up
        std::ostringstream pingCmd;
        pingCmd << "ping -c 1 -W 1 " << ip << " > /dev/null";
        if (std::system(pingCmd.str().c_str()) != 0) {
            std::cout << "[-] No response from " << ip << std::endl;
            continue;
        }

        // Get architecture
        std::ostringstream archCmd;
        archCmd << "ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no "
                << USER << "@" << ip
                << " \"/system resource print\" 2>/dev/null | grep architecture-name | awk '{print $2}'";

        FILE* pipeArch = popen(archCmd.str().c_str(), "r");
        if (!pipeArch) {
            std::cerr << "[-] Could not determine architecture on " << ip << std::endl;
            continue;
        }

        char bufferArch[64];
        std::string architecture;
        while (fgets(bufferArch, sizeof bufferArch, pipeArch) != nullptr) {
            architecture += bufferArch;
        }
        pclose(pipeArch);
        architecture.erase(architecture.find_last_not_of(" \n\r\t") + 1);

        if (architecture.empty()) {
            std::cout << "[-] Could not read architecture from " << ip << std::endl;
            continue;
        }

        std::cout << "[*] Detected architecture: " << architecture << std::endl;

        std::string packageName = "routeros-" + rosVersion + "-" + architecture + ".npk";
        std::string packageURL = "https://hidden/routeros/" + rosVersion + "/" + packageName;

        // Get current version
        std::ostringstream versionCmd;
        versionCmd << "ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no "
                   << USER << "@" << ip
                   << " \"/system package print\" 2>/dev/null | grep version | awk '{print $2}'";

        FILE* pipeVersion = popen(versionCmd.str().c_str(), "r");
        if (!pipeVersion) {
            std::cerr << "[-] Could not check version on " << ip << std::endl;
            continue;
        }

        char buffer[128];
        std::string currentVersion;
        while (fgets(buffer, sizeof buffer, pipeVersion) != nullptr) {
            currentVersion += buffer;
        }
        pclose(pipeVersion);
        currentVersion.erase(currentVersion.find_last_not_of(" \n\r\t") + 1);

        std::cout << "[*] Current version: " << currentVersion << std::endl;

        if (currentVersion == rosVersion) {
            std::cout << "[✓] Device already on version " << rosVersion << ". Skipping." << std::endl;
            continue;
        }

        // Print packages
        std::ostringstream showPackagesCmd;
        showPackagesCmd << "ssh -o StrictHostKeyChecking=no "
                        << USER << "@" << ip
                        << " \"/system package print\"";

        std::cout << "[*] Installed packages:" << std::endl;
        std::system(showPackagesCmd.str().c_str());

        // Fetch upgrade package
        std::ostringstream fetchCmd;
        fetchCmd << "ssh -o StrictHostKeyChecking=no "
                 << USER << "@" << ip
                 << " \"/tool fetch url=\\\"" << packageURL << "\\\"\"";

        std::cout << "[*] Downloading: " << packageURL << std::endl;
        std::system(fetchCmd.str().c_str());

        // Reboot device
        std::ostringstream rebootCmd;
        rebootCmd << "ssh -o StrictHostKeyChecking=no "
                  << USER << "@" << ip
                  << " \"/system reboot\"";

        std::cout << "[*] Rebooting " << ip << "..." << std::endl;
        std::system(rebootCmd.str().c_str());

        std::cout << "[✓] Done with " << ip << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }

    std::cout << "\n[✓] All devices processed.\n";
    return 0;
}
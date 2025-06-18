#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <regex>

const std::string IP_PREFIX = "10.hidden.hidden";  //
const int START_IP = 200;
const int END_IP = 200;
const std::string USER = "admin";

bool DEBUG = false;

std::string runCommand(const std::string& cmd) {
    if (DEBUG) std::cout << "[DEBUG] CMD: " << cmd << std::endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

std::vector<std::string> extractPackageNames(const std::string& packageOutput) {
    std::vector<std::string> packages;
    std::regex nameRegex("\\s*\\d+\\s+(\\S+)");
    std::smatch match;
    std::istringstream stream(packageOutput);
    std::string line;
    while (std::getline(stream, line)) {
        if (std::regex_search(line, match, nameRegex)) {
            packages.push_back(match[1]);
        }
    }
    return packages;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ros_version> [--debug]\n";
        return 1;
    }

    std::string rosVersion = argv[1];
    if (argc == 3 && std::string(argv[2]) == "--debug") {
        DEBUG = true;
        std::cout << "[DEBUG] Debug mode enabled\n";
    }

    for (int i = START_IP; i <= END_IP; ++i) {
        std::ostringstream ipStream;
        ipStream << IP_PREFIX << "." << i;
        std::string ip = ipStream.str();

        std::cout << "\n[*] Scanning " << ip << "..." << std::endl;

        if (std::system(("ping -c 1 -W 1 " + ip + " > /dev/null").c_str()) != 0) {
            std::cout << "[-] No response from " << ip << std::endl;
            continue;
        }

        std::string archCmd = "ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no " + USER + "@" + ip +
                              " \"/system resource print\" 2>/dev/null | grep architecture-name | awk '{print $2}'";
        std::string architecture = runCommand(archCmd);
        architecture.erase(architecture.find_last_not_of(" \n\r\t") + 1);

        if (architecture.empty()) {
            std::cout << "[-] Could not read architecture from " << ip << std::endl;
            continue;
        }
        std::cout << "[*] Architecture: " << architecture << std::endl;

        std::string pkgCmd = "ssh -o StrictHostKeyChecking=no " + USER + "@" + ip +
                             " \"/system package print without-paging\"";
        std::string packageOutput = runCommand(pkgCmd);

        std::vector<std::string> packageNames = extractPackageNames(packageOutput);

        std::cout << "[*] Installed packages:" << std::endl;
        for (const auto& pkg : packageNames) {
            std::cout << "    - " << pkg << std::endl;
        }

        for (const auto& pkg : packageNames) {
            std::string fileName = pkg + "-" + rosVersion + "-" + architecture + ".npk";
            std::string packageURL = "https://hidden/routeros/" + rosVersion + "/" + fileName;
            std::string fetchCmd = "ssh -o StrictHostKeyChecking=no " + USER + "@" + ip +
                                   " \"/tool fetch url=\\\"" + packageURL + "\\\"\"";
            std::cout << "[*] Fetching: " << fileName << std::endl;
            std::system(fetchCmd.c_str());
        }

        std::string rebootCmd = "ssh -o StrictHostKeyChecking=no " + USER + "@" + ip + " \"/system reboot\"";
        std::cout << "[*] Rebooting " << ip << "..." << std::endl;
        std::system(rebootCmd.c_str());

        std::cout << "[✓] Upgrade initiated for " << ip << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }

    std::cout << "\n[✓] All reachable devices processed.\n";
    return 0;
}

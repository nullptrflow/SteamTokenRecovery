# SteamTokenRecovery 🔑

A high-performance C++ utility designed for local Steam account session recovery. This tool demonstrates the use of Windows DPAPI and CRC32 checksums to retrieve and decrypt Steam connection tokens for educational purposes and authorized security research.

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)
![Windows](https://img.shields.io/badge/Platform-Windows-blue?style=for-the-badge&logo=windows&logoColor=white)

> [!CAUTION]
> **Disclaimer:** This project is for educational and authorized research purposes only. Accessing account data without explicit permission is illegal. Use this software responsibly and only on your own accounts.

---

### Features 🚀

- **DPAPI Decryption**: Uses the Windows Data Protection API to decrypt encrypted Steam tokens.
- **VDF Parsing**: Lightweight parsing of Valve Data Format files (`loginusers.vdf`, `local.vdf`).
- **CRC32 Implementation**: Custom CRC32 algorithm used to generate Steam's `ConnectCache` keys.
- **Zero Dependencies**: Pure C++ with standard Windows libraries (`Crypt32.lib`).

### How It Works 🛠️

1. **Locates Steam Files**: Automatically finds `loginusers.vdf` and `local.vdf` in the standard Steam installation paths.
2. **Generates Cache Keys**: Computes the CRC32 hash of the account name to identify the corresponding encrypted token in the `MachineUserConfigStore`.
3. **Decrypts Tokens**: Passes the encrypted hex data and the account name (as entropy) to `CryptUnprotectData`.
4. **Outputs Credentials**: Displays the recovered session tokens in a clear format.

### Quick Start ⚡

1. **Build the Project**:
   - Open in **Visual Studio 2022**.
   - Link `Crypt32.lib` in the project settings.
   - Build for **x64 Release**.

2. **Run**:
   ```bash
   SteamTokenRecovery.exe
   ```

---

Created by [nullptrflow](https://github.com/nullptrflow)

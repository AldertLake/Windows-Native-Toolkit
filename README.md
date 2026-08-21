<img src="https://aldertlake-docs.vercel.app/product-icons/wnt.svg" width="50">

# Windows Native Toolkit (WNT)

[![Stars](https://img.shields.io/github/stars/AldertLake/Windows-Native-Toolkit?style=for-the-badge&logo=github)](https://github.com/AldertLake/Windows-Native-Toolkit/stargazers)
[![Last Updated](https://img.shields.io/github/last-commit/AldertLake/Windows-Native-Toolkit?style=for-the-badge&label=last%20updated)](https://github.com/AldertLake/Windows-Native-Toolkit/commits)

**Windows Native Toolkit** is a high-performance Unreal Engine plugin that bridges the gap between your project and the underlying operating system. It provides comprehensive Blueprint and C++ access to native system features, hardware diagnostics, and advanced OS workflows that are typically inaccessible within the standard engine API.



## Community & Support
- **Documentation:** [WNT Documentation Portal](https://aldertlake-docs.vercel.app/docs/wnt)
- **Discord:** [Join our Community](https://discord.gg/QpPPfh6WVn)
- **Marketplace:** [Fab / Unreal Engine Marketplace](https://www.fab.com/listings/db1cb6ed-ac7e-4408-a901-e45d6694cb0b)
- **Issues:** Please use the [GitHub Issues](https://github.com/AldertLake/Windows-Native-Toolkit/issues) tab for bug reports and feature requests.



## Key Features

### System & Hardware Intelligence
*   **Real-time Diagnostics:** Monitor CPU vendor, physical/logical cores, and system-wide utilization.
*   **Advanced GPU Tracking:** Query multiple adapters, track dedicated vs. shared VRAM, and monitor process-specific VRAM consumption.
*   **Process Control:** Relaunch the game with custom command-line arguments or perform safe "Force Kill" operations in packaged builds.
*   **Async CLI:** Run CMD or PowerShell commands asynchronously with background output capture and administrator elevation support (UAC).

### Native Config Save System
*   **AES-256 Encryption:** Protect sensitive local data with project-wide encrypted string nodes.
*   **Wildcard Serialization:** "Write/Read Config Value" nodes that automatically handle data type conversion for any Blueprint variable.
*   **Native Arrays:** Support for Unreal's `+Key=Value` repeated config format with native `Add Unique` and `Remove` logic.
*   **Asset Path Stability:** Dedicated helpers to convert between Soft Object/Class references and stable config strings.

### Network & Connectivity
*   **Connection Awareness:** Detect active connection types (Ethernet, Wi-Fi, or both) and adapter hardware descriptions.
*   **Wi-Fi Intelligence:** Retrieve SSIDs, BSSIDs, Signal Quality (0-100), and Authentication Algorithms.
*   **Async Network Tasks:** Perform non-blocking Pings, DNS resolutions, and Public IP lookups via multiple providers (Amazon, IfConfig, ICanHazIP).

### Display & Monitor Management
*   **Multi-Monitor Support:** Enumerate all connected monitors and identify which one currently hosts the game window.
*   **Native Resolution Detection:** Query the OS-recommended "Native" resolution for any display.
*   **Dynamic Scaling:** Apply combined Resolution and Refresh Rate changes with an optional "Auto-Revert" safety timer.

### Advanced File I/O
*   **Async Folder Scanning:** Calculate directory sizes on background threads to prevent UI hitches during large file operations.
*   **Native File Pickers:** Standard Windows dialogs for picking files or folders.
*   **Project Settings Integration:** Configure global defaults for timeouts, encryption keys, and default file paths directly in the Unreal Editor.

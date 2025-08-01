# Changelog

All notable changes to TinyRequest will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Nothing yet

### Changed
- Nothing yet

### Fixed
- Nothing yet

## [1.0.0] - 2025-08-01

### Added
- Initial project setup and documentation

### Changed
- Nothing yet

### Deprecated
- Nothing yet

### Removed
- Nothing yet

### Fixed
- Nothing yet

### Security
- Nothing yet

## [1.0.0] - 2025-01-08

### Added
- **Core HTTP Client**
  - Support for all standard HTTP methods (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)
  - Custom header management with dynamic lists
  - Request body support for POST/PUT/PATCH requests
  - SSL/TLS support with configurable verification
  - Response time measurement and status code handling
  - Comprehensive error handling with user-friendly messages

- **Authentication System**
  - API Key authentication (header or query parameter)
  - Bearer Token authentication
  - Basic Authentication (username/password)
  - OAuth 2.0 token support
  - Per-request authentication enable/disable toggles

- **Collections Management**
  - Request organization into collections (like Postman workspaces)
  - Collection creation, renaming, and deletion
  - Request creation, duplication, and management within collections
  - Automatic collection persistence to JSON files
  - Import/Export functionality for sharing collections

- **Smart Cookie Management**
  - Automatic cookie storage per collection
  - Cookie expiration handling and cleanup
  - Domain and path-based cookie matching
  - Secure cookie support for HTTPS requests
  - Session persistence across requests

- **User Interface**
  - Clean three-tab interface (Collections, Request, Response)
  - Modern Dear ImGui-based UI with custom theming
  - Real-time response display with syntax highlighting
  - Collapsible panels for headers and request body
  - Font Awesome icons for better visual experience

- **Data Persistence**
  - Auto-save functionality with configurable intervals
  - JSON-based storage format
  - Automatic backup and recovery systems
  - Settings persistence (auto-save preferences, SSL settings)
  - Legacy data migration support

- **Build System**
  - CMake-based build system with cross-platform support
  - Automated dependency fetching script
  - Support for multiple Linux distributions (Ubuntu, Fedora, Arch, openSUSE)
  - Debug and Release build configurations
  - Debian package generation

- **Developer Features**
  - Comprehensive error logging and debugging
  - Memory leak prevention and validation
  - Response size limits to prevent memory exhaustion
  - Progress callbacks for large requests
  - Configurable SSL verification for development/testing

### Technical Details
- **Languages**: C11 for core functionality, C++17 for UI components
- **Dependencies**: Dear ImGui v1.92.1, cJSON v1.7.15, libcurl, GLFW3
- **Platform**: Linux x86_64 (Ubuntu, Fedora, Arch Linux, openSUSE)
- **Memory Usage**: ~50MB typical usage
- **Installation Size**: ~10MB

### Known Issues
- None reported yet

---

## Version History Summary

- **v1.0.0** (2025-01-08) - Initial release with full HTTP client functionality
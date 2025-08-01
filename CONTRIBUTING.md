# Contributing to TinyRequest

Thank you for your interest in contributing to TinyRequest! I welcome contributions from developers of all skill levels. This document provides guidelines and information to help you contribute effectively.

## 🚀 Getting Started

### Prerequisites

- **Linux Environment** (Ubuntu, Fedora, Arch, openSUSE)
- **C11 Compiler** (GCC or Clang)
- **C++17 Compiler** (GCC or Clang)
- **CMake** 3.16 or higher
- **Git** for version control

### Setting Up Development Environment

1. **Fork and Clone**
   ```bash
   git clone https://github.com/dexter-xd/TinyRequest.git
   cd TinyRequest
   ```

2. **Install Dependencies**
   ```bash
   ./scripts/fetch_dependencies.sh
   ```

3. **Build and Test**
   ```bash
   ./scripts/build.sh
   ./cmake-build-debug/bin/TinyRequest
   ```

## 📋 How to Contribute

### Reporting Bugs

Before creating a bug report, please:
- Check existing issues to avoid duplicates
- Test with the latest version
- Gather relevant system information

**Bug Report Template:**
```markdown
**Environment:**
- OS: [e.g., Ubuntu 22.04]
- Architecture: [e.g., x86_64]
- TinyRequest Version: [e.g., v1.0.0]

**Steps to Reproduce:**
1. Step one
2. Step two
3. Step three

**Expected Behavior:**
What you expected to happen

**Actual Behavior:**
What actually happened

**Additional Context:**
Screenshots, logs, or other relevant information
```

### Suggesting Features

Feature requests are welcome! Please:
- Check if the feature already exists or is planned
- Explain the use case and benefits
- Consider implementation complexity
- Be open to discussion and alternatives

### Code Contributions

#### Types of Contributions I Welcome

- 🐛 **Bug fixes**
- ✨ **New features**
- 📚 **Documentation improvements**
- 🎨 **UI/UX enhancements**
- ⚡ **Performance optimizations**
- 🧪 **Test coverage improvements**
- 🔧 **Build system enhancements**

#### Pull Request Process

1. **Create a Branch**
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/bug-description
   ```

2. **Make Your Changes**
   - Follow the coding standards (see below)
   - Write clear, concise commit messages
   - Add tests if applicable
   - Update documentation as needed

3. **Test Your Changes**
   ```bash
   # Build and test
   ./scripts/build.sh
   
   # Test on different distributions if possible
   # Run memory checks
   valgrind --leak-check=full ./cmake-build-debug/bin/TinyRequest
   ```

4. **Commit Your Changes**
   ```bash
   git add .
   git commit -m "feat: add new authentication method"
   # or
   git commit -m "fix: resolve memory leak in HTTP client"
   ```

5. **Push and Create PR**
   ```bash
   git push origin feature/your-feature-name
   ```
   Then create a Pull Request on GitHub.

## 📝 Coding Standards

### C Code Style

```c
// Use snake_case for functions and variables
int http_client_send_request(HttpClient* client, const Request* request);

// Use PascalCase for structs and typedefs
typedef struct {
    char name[128];
    char value[512];
} Header;

// Constants in UPPER_CASE
#define MAX_HEADER_SIZE 512

// Function documentation
/**
 * Sends an HTTP request and populates the response
 * @param client The HTTP client instance
 * @param request The request to send
 * @param response The response structure to populate
 * @return 0 on success, -1 on error
 */
int http_client_send_request(HttpClient* client, const Request* request, Response* response);
```

### C++ Code Style

```cpp
// Use camelCase for methods and variables in C++ UI code
void renderMainWindow();
bool handleUserInput();

// Use PascalCase for classes
class UIManager {
public:
    void render();
private:
    bool isVisible;
};

// Namespace usage
namespace ui {
    void renderPanel();
}
```

### General Guidelines

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: 100 characters maximum
- **Braces**: Opening brace on same line for functions, new line for control structures
- **Comments**: Use `//` for single line, `/* */` for multi-line
- **Memory Management**: Always check for NULL pointers, free allocated memory
- **Error Handling**: Use return codes, provide meaningful error messages

## 🧪 Testing Guidelines

### Manual Testing

- Test on multiple Linux distributions
- Verify all HTTP methods work correctly
- Test authentication mechanisms
- Check collection management features
- Validate import/export functionality

### Memory Testing

```bash
# Check for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./cmake-build-debug/bin/TinyRequest

# Check for memory errors
valgrind --tool=memcheck ./cmake-build-debug/bin/TinyRequest
```

### Performance Testing

- Monitor memory usage during large requests
- Test with slow network connections
- Verify UI responsiveness during network operations

## 📚 Documentation

### Code Documentation

- Document all public APIs
- Explain complex algorithms
- Add examples for non-obvious usage
- Keep comments up-to-date with code changes

### User Documentation

- Update README.md for new features
- Add usage examples
- Document configuration options
- Include troubleshooting information

## 🔄 Development Workflow

### Branch Naming

- `feature/feature-name` - New features
- `fix/bug-description` - Bug fixes
- `docs/update-description` - Documentation updates
- `refactor/component-name` - Code refactoring
- `test/test-description` - Test improvements

### Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
type(scope): description

feat(auth): add OAuth 2.0 support
fix(http): resolve memory leak in response handling
docs(readme): update installation instructions
style(ui): improve button spacing
refactor(collections): simplify cookie management
test(http): add unit tests for request validation
```

### Code Review Process

1. **Automated Checks**: All PRs must pass automated builds
2. **Manual Review**: I will review all pull requests
3. **Testing**: Verify functionality works as expected
4. **Documentation**: Ensure docs are updated if needed

## 🏗️ Project Architecture

TinyRequest is built with a modular, performance-focused architecture using C11 and C++17:

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   UI Layer      │    │  Application     │    │  Data Layer     │
│  (Dear ImGui)   │◄──►│     State        │◄──►│ (Collections)   │
│                 │    │   Management     │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ Window System   │    │  HTTP Client     │    │  Persistence    │
│   (GLFW)        │    │   (libcurl)      │    │    (JSON)       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### Key Components

- **Application Core** (C++) - Lifecycle management and main event loop
- **State Manager** (C) - Centralized state with automatic synchronization
- **HTTP Client** (C) - libcurl wrapper with advanced features
- **Collections System** (C) - Request organization and cookie management
- **Persistence Layer** (C) - cJSON-based storage with auto-save
- **UI System** (C++) - Modular Dear ImGui-based interface

### Project Structure
```
TinyRequest/
├── src/                    # Source code
│   ├── app/               # Application core (C++)
│   ├── ui/                # User interface components (C++)
│   └── *.c                # Core functionality (C)
├── include/               # Header files
├── assets/                # Icons and fonts
├── scripts/               # Build and setup scripts
├── externals/             # External dependencies (fetched by script)
└── cmake-build-*/         # Build outputs
```

### Dependencies

**External Libraries:**
- [Dear ImGui](https://github.com/ocornut/imgui) v1.92.1 - Immediate mode GUI (C++)
- [cimgui](https://github.com/cimgui/cimgui) v1.53.1 - C bindings for ImGui
- [cJSON](https://github.com/DaveGamble/cJSON) v1.7.15 - JSON parsing (C)
- [libcurl](https://curl.se/libcurl/) - HTTP networking (C)
- [GLFW3](https://www.glfw.org/) - Window management (C)
- [OpenGL](https://www.opengl.org/) - Graphics rendering

**System Dependencies:**
- CMake 3.16+ for build system
- GCC/Clang with C11 and C++17 support
- pkg-config for dependency detection
- Linux development headers (X11, OpenGL, etc.)

## 🏗️ Architecture Guidelines

### Adding New Features

1. **Plan the Architecture**: Consider how it fits with existing code
2. **Update State Management**: Modify `AppState` if needed
3. **UI Integration**: Add UI components in appropriate modules
4. **Persistence**: Update JSON serialization if data needs saving
5. **Testing**: Ensure feature works across different scenarios

### Modifying Existing Code

- Maintain backward compatibility when possible
- Update related documentation
- Consider performance implications
- Test edge cases thoroughly

## 🤝 Community Guidelines

### Communication

- Be respectful and constructive
- Ask questions if something is unclear
- Help other contributors when possible
- Use GitHub Discussions for general questions

### Code of Conduct

I follow the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/). Please read and follow these guidelines to ensure a welcoming environment for all contributors.

## 📞 Getting Help

- **GitHub Issues**: For bugs and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Code Review**: Ask for feedback on your approach before implementing large changes

## 🎯 Good First Issues

Looking for a place to start? Look for issues labeled:
- `good first issue` - Perfect for newcomers
- `help wanted` - Community input needed
- `documentation` - Improve docs and examples
- `bug` - Fix existing issues

## 📈 Recognition

Contributors will be:
- Listed in the project's contributors section
- Mentioned in release notes for significant contributions
- Acknowledged in the project documentation

---

Thank you for contributing to TinyRequest! Your efforts help me make this tool better for the entire Linux community. 🚀
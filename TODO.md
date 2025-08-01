# TinyRequest TODO

This document tracks planned features, improvements, and known issues for TinyRequest.

> 💡 **New contributors welcome!** If you're interested in working on any of these items, please check [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines and feel free to open an issue to discuss your approach.

## 🚀 High Priority

### Core Features
- [ ] **Request History** - Keep track of recently sent requests
- [ ] **Environment Variables** - Support for dynamic values in URLs and headers
- [ ] **Request Templates** - Save and reuse common request patterns
- [ ] **Bulk Operations** - Send multiple requests in sequence
- [ ] **Request Validation** - Pre-flight validation of URLs and headers

### User Experience
- [ ] **Keyboard Shortcuts** - Add hotkeys for common operations
- [ ] **Search Functionality** - Search through collections and requests
- [ ] **Request Duplication** - Easy copying of requests within/between collections
- [ ] **Undo/Redo System** - Revert changes to requests and collections
- [ ] **Dark/Light Theme Toggle** - User-selectable UI themes

### Performance
- [ ] **Async Request Handling** - Non-blocking UI during network operations
- [ ] **Request Cancellation** - Ability to cancel in-progress requests
- [ ] **Connection Pooling** - Reuse connections for better performance
- [ ] **Memory Optimization** - Reduce memory footprint for large responses

## 🎯 Medium Priority

### Advanced Features
- [ ] **GraphQL Support** - Specialized support for GraphQL queries
- [ ] **WebSocket Testing** - Support for WebSocket connections
- [ ] **File Upload Support** - Multipart form data with file attachments
- [ ] **Response Filtering** - JSONPath/XPath filtering of responses
- [ ] **Request Chaining** - Use response data in subsequent requests

### Authentication Enhancements
- [ ] **OAuth 2.0 Flow** - Complete OAuth authorization flow
- [ ] **JWT Token Parsing** - Decode and display JWT token contents
- [ ] **Certificate Authentication** - Client certificate support
- [ ] **NTLM Authentication** - Windows domain authentication
- [ ] **Digest Authentication** - HTTP Digest auth support

### Data Management
- [ ] **Collection Sharing** - Cloud sync or team sharing features
- [ ] **Version Control** - Track changes to collections over time
- [ ] **Backup/Restore** - Manual backup and restore functionality
- [ ] **Data Encryption** - Encrypt sensitive data at rest
- [ ] **Collection Templates** - Predefined collection structures

### Developer Tools
- [ ] **Code Generation** - Generate code snippets in various languages
- [ ] **API Documentation** - Generate docs from collections
- [ ] **Mock Server** - Built-in mock server for testing
- [ ] **Request Validation** - Schema validation for requests/responses
- [ ] **Performance Profiling** - Detailed timing and performance metrics

## 🔧 Low Priority

### UI/UX Improvements
- [ ] **Customizable Layout** - Resizable and movable panels
- [ ] **Response Visualization** - Charts and graphs for JSON data
- [ ] **Syntax Highlighting** - Better highlighting for different content types
- [ ] **Font Size Controls** - User-adjustable font sizes
- [ ] **Window Management** - Multiple windows support

### Platform Support
- [ ] **macOS Port** - Native macOS version
- [ ] **Windows Port** - Native Windows version using Win32 API
- [ ] **ARM64 Support** - Support for ARM-based Linux systems
- [ ] **Wayland Support** - Native Wayland compositor support

### Integration Features
- [ ] **CLI Interface** - Command-line version for automation
- [ ] **Plugin System** - Support for third-party extensions
- [ ] **External Tool Integration** - Integration with curl, httpie, etc.
- [ ] **CI/CD Integration** - Export for use in automated testing
- [ ] **Browser Extension** - Capture requests from web browsers

### Advanced Networking
- [ ] **HTTP/2 Support** - Full HTTP/2 protocol support
- [ ] **HTTP/3 Support** - QUIC-based HTTP/3 support
- [ ] **Proxy Support** - HTTP/SOCKS proxy configuration
- [ ] **Custom DNS** - Custom DNS server configuration
- [ ] **Network Debugging** - Detailed network trace information

## 🐛 Known Issues

### Current Bugs
- [ ] None reported yet

### Potential Issues
- [ ] **Memory Usage** - Monitor memory usage with very large responses
- [ ] **SSL Certificates** - Test with various SSL certificate configurations
- [ ] **Unicode Support** - Ensure proper handling of international characters
- [ ] **File Permissions** - Handle cases where config directory is read-only

## 📋 Technical Debt

### Code Quality
- [ ] **Unit Tests** - Add comprehensive unit test suite
- [ ] **Integration Tests** - Add end-to-end testing
- [ ] **Code Coverage** - Achieve >80% code coverage
- [ ] **Static Analysis** - Regular static analysis with cppcheck/clang-tidy
- [ ] **Memory Leak Testing** - Regular valgrind testing

### Documentation
- [ ] **API Documentation** - Document all public APIs
- [ ] **Architecture Guide** - Detailed architecture documentation
- [ ] **User Manual** - Comprehensive user guide
- [ ] **Video Tutorials** - Create usage tutorials
- [ ] **FAQ Section** - Common questions and answers

### Build System
- [ ] **Automated Testing** - GitHub Actions CI/CD pipeline
- [ ] **Cross-compilation** - Build for multiple architectures
- [ ] **Package Repositories** - Submit to official Linux repositories
- [ ] **AppImage/Flatpak** - Universal Linux packaging
- [ ] **Reproducible Builds** - Ensure build reproducibility

## 🎯 Version Roadmap

### v1.1.0 - Enhanced User Experience
- Request history
- Environment variables
- Keyboard shortcuts
- Search functionality
- Async request handling

### v1.2.0 - Advanced Features
- GraphQL support
- File upload support
- OAuth 2.0 flow
- Request chaining
- Code generation

### v1.3.0 - Developer Tools
- Mock server
- API documentation generation
- Performance profiling
- Plugin system
- CLI interface

### v2.0.0 - Major Overhaul
- Multi-platform support
- HTTP/2 support
- Advanced networking features
- Complete UI redesign
- Cloud synchronization

## 📝 Notes

### Implementation Priority
1. **User-requested features** - Based on GitHub issues and feedback
2. **Core stability** - Bug fixes and performance improvements
3. **Developer experience** - Tools that help API development workflow
4. **Platform expansion** - Support for more operating systems

### Contribution Welcome
Items marked with 🤝 are particularly suitable for community contributions:
- 🤝 Documentation improvements
- 🤝 UI/UX enhancements
- 🤝 Additional authentication methods
- 🤝 Platform-specific features
- 🤝 Testing and quality assurance

**Want to contribute?** Please read [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines on:
- Setting up the development environment
- Coding standards and style guide
- Pull request process
- Testing requirements

---

**Last Updated**: January 8, 2025  
**Next Review**: February 8, 2025

> This TODO list is a living document. Priorities may change based on user feedback, technical constraints, and development resources.
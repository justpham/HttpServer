# Code Style and Linting Guide

This project uses automated code linting and formatting to maintain consistent code style across all C source files.

## Tools Used

### 1. clang-format
- **Purpose**: Automatic code formatting
- **Configuration**: `.clang-format` file in the project root
- **Style**: Based on GNU style with project-specific customizations

### 2. cppcheck
- **Purpose**: Static code analysis to catch potential bugs, security issues, and style violations
- **Configuration**: `.cppcheck-suppressions` file for suppressing false positives
- **Standards**: C99 compliance checking

## Available Make Targets

### Formatting
```bash
# Format all C source files automatically
make format

# Check if code is properly formatted (used in CI/pre-commit)
make format-check
```

### Static Analysis
```bash
# Run cppcheck static analysis
make cppcheck

# Generate HTML report with detailed analysis
make cppcheck-report

# Run all linting checks (format-check + cppcheck)
make lint
```

## VS Code Integration

The project includes VS Code settings (`.vscode/settings.json`) that:
- Enable format-on-save using clang-format
- Configure C/C++ extension for proper IntelliSense
- Set up cppcheck integration for real-time analysis

### Recommended VS Code Extensions
1. **C/C++** (ms-vscode.cpptools) - Core C/C++ support
2. **C/C++ Extension Pack** (ms-vscode.cpptools-extension-pack) - Additional tools
3. **Cppcheck** (matthewferreira.cppcheck) - Static analysis integration

## Pre-commit Hooks

A pre-commit script is available in `scripts/pre-commit` that:
- Runs format checking
- Performs static analysis
- Prevents commits if linting fails

To install the pre-commit hook:
```bash
# If using git
cp scripts/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Code Style Guidelines

The clang-format configuration enforces:
- 4-space indentation (no tabs)
- 100-character line limit
- GNU-style bracing with custom tweaks
- Consistent spacing around operators
- Proper pointer alignment (right-aligned: `char *ptr`)

## Excluding Files

Third-party code (like cJSON) is automatically excluded from formatting and some static analysis checks to avoid modifying external dependencies.

## CI Integration

Add the following to your CI pipeline:
```bash
make lint  # This will fail the build if linting issues are found
```

## Troubleshooting

### Format Check Failures
Run `make format` to automatically fix formatting issues, then commit the changes.

### Static Analysis Warnings
Review cppcheck output carefully. If you encounter false positives, you can:
1. Add specific suppressions to `.cppcheck-suppressions`
2. Use inline suppressions: `// cppcheck-suppress warningId`

### VS Code Not Formatting
Ensure the C/C++ extension is installed and the workspace settings are properly loaded.

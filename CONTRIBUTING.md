# Contributing to ShipOS

Thank you for your interest in contributing to ShipOS! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and professional environment. Be kind, constructive, and considerate in all interactions.

## Before You Begin

### Contributor License Agreement (CLA)

**IMPORTANT:** Before your first contribution, you must sign the [Contributor License Agreement (CLA)](CLA.md).

The CLA ensures that:
- All contributions become the exclusive property of ShipOS
- ShipOS retains full rights to use, modify, distribute, and commercialize your contributions
- You grant ShipOS the necessary rights to incorporate your work

**How to sign the CLA:**

1. Read the [CLA.md](CLA.md) document carefully
2. Open a pull request to the ShipOS repository
3. The CLA Assistant bot will automatically prompt you to sign the CLA
4. Follow the instructions provided by the bot to complete the signing process
5. Once signed, your pull request will be eligible for review

**Note:** You only need to sign the CLA once. After signing, all your future contributions will be covered by the agreement.

## Development Workflow

### 1. Fork and Clone

```bash
# Fork the repository on GitHub, then:
git clone https://github.com/YOUR_USERNAME/ShipOS.git
cd ShipOS
```

### 2. Create a Branch

```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/your-bug-fix
```

### 3. Make Your Changes

- Follow the existing code style (see [Code Style](#code-style))
- Write clear, documented code
- Add comments where necessary
- Test your changes thoroughly

### 4. Commit Your Changes

```bash
git add .
git commit -m "Description of your changes"
```

**Commit Message Guidelines:**
- Use clear, descriptive commit messages
- Start with a capital letter
- Use present tense ("Add feature" not "Added feature")
- Keep the first line under 72 characters
- Add a detailed description if needed

### 5. Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

Then create a pull request on GitHub.

## Code Style

### C Code Style

ShipOS uses **Allman style** for code formatting:

- Opening braces on a new line
- 4-space indentation (no tabs)
- Right-aligned pointers: `type *name`
- Spaces around operators and after keywords

**Example:**
```c
void example_function(int *ptr, int value)
{
    if (ptr == 0)
    {
        return;
    }
    
    for (int i = 0; i < value; i++)
    {
        ptr[i] = i;
    }
}
```

### Formatting Your Code

We provide a code formatter to ensure consistency:

```bash
# Format all files
./scripts/format_code.sh

# Format specific files
./scripts/format_code.sh kernel/main.c

# Check if files are formatted correctly
./scripts/format_code.sh --check
```

**Before submitting a PR, please format your code using the provided script.**

### Include Guidelines

- Include headers explicitly (no transitive includes)
- Group includes: system headers, then project headers
- Use relative paths for project headers
- Keep includes organized and minimal

### Documentation

- Use Doxygen-style comments for functions
- Document all public APIs
- Explain complex algorithms or non-obvious code
- Keep comments up-to-date with code changes

## Testing

Before submitting a pull request:

1. **Build the kernel:**
   ```bash
   make clean
   make build_kernel
   ```

2. **Test in QEMU:**
   ```bash
   make qemu-headless
   ```

3. **Verify no compilation warnings** (except known ones)

4. **Test your specific changes** thoroughly

## Pull Request Process

### Before Submitting

- [ ] Code follows the project's style guidelines
- [ ] Code is formatted using `./scripts/format_code.sh`
- [ ] All tests pass and kernel builds successfully
- [ ] CLA has been signed (checked automatically by CLA Assistant)
- [ ] Changes are documented (if applicable)
- [ ] Commit messages are clear and descriptive

### PR Description Template

When creating a pull request, please include:

```markdown
## Description
Brief description of what this PR does.

## Changes
- List of specific changes made
- Any breaking changes

## Testing
How you tested these changes.

## Related Issues
Closes #(issue number) if applicable
```

### Review Process

1. **CLA Check:** The CLA Assistant bot will verify you've signed the CLA
2. **Automated Checks:** CI will run build and test checks
3. **Code Review:** Maintainers will review your code
4. **Feedback:** Address any requested changes
5. **Merge:** Once approved, your PR will be merged

## What to Contribute

We welcome contributions in the following areas:

- **Bug Fixes:** Fix issues reported in the issue tracker
- **Features:** Implement features from the roadmap
- **Documentation:** Improve documentation and comments
- **Code Quality:** Refactoring and code improvements
- **Tests:** Add or improve test coverage

## Getting Help

- **Issues:** Check existing issues before creating new ones
- **Discussions:** Use GitHub Discussions for questions
- **Documentation:** Read the README and code comments

## License

By contributing to ShipOS, you agree that your contributions will be licensed under the terms of the [CLA](CLA.md), which grants ShipOS exclusive ownership and all rights to your contributions.

---

Thank you for contributing to ShipOS!


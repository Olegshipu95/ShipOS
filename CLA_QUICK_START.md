# CLA Quick Start Guide

This guide will help you quickly set up the Contributor License Agreement (CLA) system for ShipOS.

## What You Get

✅ **CLA.md** - The Contributor License Agreement document  
✅ **CONTRIBUTING.md** - Contribution guidelines  
✅ **GitHub Actions Workflow** - Automated CLA checking  
✅ **CLA Assistant Configuration** - Ready for GitHub App integration  
✅ **PR Template** - Reminds contributors about CLA  
✅ **Issue Templates** - Bug reports and feature requests  

## Quick Setup (5 Minutes)

### Step 1: Install CLA Assistant App

1. Go to: **https://github.com/apps/cla-assistant**
2. Click **"Configure"** or **"Install"**
3. Select your repository: **ShipOS**
4. Grant necessary permissions:
   - Read access to pull requests
   - Write access to pull requests (for status updates)
   - Read access to repository metadata

### Step 2: Configure CLA Assistant

After installation:

1. Go to your repository on GitHub
2. Navigate to: **Settings → Integrations → Installed GitHub Apps → CLA Assistant**
3. Click **"Configure"**
4. Set the following:
   - **CLA Document Path:** `CLA.md`
   - **Signature Storage:** Choose a location (recommended: separate repository)
   - **Require Re-signing on Update:** `false` (unless you want contributors to re-sign when CLA changes)

### Step 3: Test It

1. Create a test pull request (from a fork or different branch)
2. The CLA Assistant bot should automatically comment asking you to sign
3. Click the link to sign the CLA
4. Verify the PR status updates to show CLA is signed ✅

### Step 4: Set Up Branch Protection (Optional but Recommended)

1. Go to: **Settings → Branches**
2. Add rule for `main` branch
3. Enable: **"Require status checks to pass before merging"**
4. Add check: **`cla/signature`**
5. Save changes

This prevents merging PRs without signed CLAs.

## Files Created

### Core Documents

- **`CLA.md`** - The Contributor License Agreement
  - Grants you exclusive ownership of all contributions
  - Covers past, present, and future contributions
  - Allows you to close, sell, or restrict the repository

- **`CONTRIBUTING.md`** - Contribution guidelines
  - Explains the contribution process
  - Documents code style requirements
  - Links to CLA

### GitHub Integration

- **`.github/workflows/cla-check.yml`** - GitHub Actions workflow
  - Checks CLA status on pull requests
  - Comments on PRs requiring CLA
  - Updates PR status

- **`.github/cla.yml`** - CLA Assistant configuration
  - Configures the CLA Assistant app
  - Sets up signature storage

- **`.github/PULL_REQUEST_TEMPLATE.md`** - PR template
  - Reminds contributors about CLA
  - Includes checklist

- **`.github/ISSUE_TEMPLATE/`** - Issue templates
  - Bug report template
  - Feature request template

### Documentation

- **`.github/CLA_SETUP.md`** - Detailed setup instructions
- **`.github/MAINTAINERS.md`** - Guide for maintainers
- **`scripts/verify_cla.sh`** - Helper script for manual verification

## How It Works

### For Contributors

1. Contributor opens a pull request
2. CLA Assistant bot automatically comments asking to sign CLA
3. Contributor clicks link and signs the CLA
4. PR status updates to show CLA is signed ✅
5. PR can now be reviewed and merged

### For Maintainers

1. PR opens → CLA check runs automatically
2. If CLA not signed → Bot comments with instructions
3. If CLA signed → Status shows ✅, PR ready for review
4. Branch protection (if enabled) prevents merging without CLA

## Key Features

✅ **Automatic Verification** - CLA Assistant checks signatures automatically  
✅ **PR Status Updates** - Clear visual indication of CLA status  
✅ **Retroactive Application** - CLA covers all past contributions  
✅ **Exclusive Ownership** - All contributions become your property  
✅ **Full Control** - You can close, sell, or restrict the repository  

## Important Notes

⚠️ **CLA is Legally Binding** - Make sure contributors understand what they're signing

⚠️ **Retroactive** - The CLA applies to all past contributions once signed

⚠️ **Exclusive Ownership** - Contributors give up all rights to their contributions

⚠️ **No Exceptions** - Don't merge PRs without signed CLA, even for small changes

## Next Steps

1. ✅ Install CLA Assistant App (see Step 1 above)
2. ✅ Configure CLA Assistant (see Step 2 above)
3. ✅ Test with a sample PR
4. ✅ Set up branch protection (optional)
5. ✅ Update README if needed
6. ✅ Review and customize CLA.md if necessary

## Customization

### Update CLA.md

You may want to customize:
- **Governing Law** (line 9) - Specify your jurisdiction
- **Project Owner Name** - Add your name/entity
- **Contact Information** - Add contact details

### Update CONTRIBUTING.md

Customize:
- Code style guidelines
- Testing requirements
- Review process
- Communication channels

## Support

- **CLA Setup Issues:** See [CLA_SETUP.md](.github/CLA_SETUP.md)
- **Maintainer Questions:** See [MAINTAINERS.md](.github/MAINTAINERS.md)
- **CLA Assistant Help:** https://github.com/cla-assistant/github-action

---

**You're all set!** Once you install the CLA Assistant app, the system will automatically handle CLA verification for all pull requests.


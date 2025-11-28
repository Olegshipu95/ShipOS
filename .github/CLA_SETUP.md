# CLA Assistant Setup Guide

This guide explains how to set up CLA Assistant for automatic CLA verification on pull requests.

## What is CLA Assistant?

CLA Assistant is a GitHub App that automatically:
- Prompts contributors to sign the CLA when they open a PR
- Verifies that contributors have signed the CLA
- Updates PR status based on CLA signature status
- Stores signatures securely

## Setup Instructions

### Option 1: Using CLA Assistant GitHub App (Recommended)

1. **Install the CLA Assistant App:**
   - Go to: https://github.com/apps/cla-assistant
   - Click "Configure" or "Install"
   - Select your repository (ShipOS)
   - Grant the necessary permissions

2. **Configure CLA Assistant:**
   - After installation, go to your repository settings
   - Navigate to "CLA Assistant" in the left sidebar
   - Configure the following:
     - **CLA Document Path:** `CLA.md`
     - **Signature Storage:** Choose where to store signatures (recommended: separate repository or branch)
     - **Require Re-signing on Update:** Set to `false` (unless you want contributors to re-sign when CLA changes)

3. **Test the Setup:**
   - Open a test pull request
   - The CLA Assistant bot should automatically comment asking you to sign
   - Sign the CLA through the provided link
   - The PR status should update to show CLA is signed

### Option 2: Manual CLA Verification (Current Implementation)

The repository includes a GitHub Actions workflow (`.github/workflows/cla-check.yml`) that:
- Checks for CLA signatures on pull requests
- Comments on PRs requiring CLA signature
- Updates PR status

**Limitations:**
- Requires manual verification of signatures
- Less automated than CLA Assistant App
- Signatures need to be stored and checked manually

**To use this approach:**
1. The workflow is already configured
2. Contributors should comment on PRs to sign the CLA
3. Maintainers need to verify signatures manually

### Option 3: Hybrid Approach

You can use both:
- CLA Assistant App for automatic verification (primary)
- GitHub Actions workflow as a backup check

## Configuration Files

### `.github/cla.yml`
Configuration file for CLA Assistant App (if using the app).

### `.github/workflows/cla-check.yml`
GitHub Actions workflow for manual CLA checking (backup/alternative).

## Signature Storage

### Recommended: Separate Repository

Create a separate private repository (e.g., `shipos-cla-signatures`) to store signatures:

```
shipos-cla-signatures/
├── signatures/
│   ├── username1.json
│   ├── username2.json
│   └── ...
└── README.md
```

Each signature file contains:
```json
{
  "username": "contributor-username",
  "email": "contributor@example.com",
  "signedAt": "2024-01-15T10:30:00Z",
  "claVersion": "1.0",
  "ipAddress": "xxx.xxx.xxx.xxx"
}
```

### Alternative: Branch in Same Repository

Store signatures in a `cla-signatures` branch:
- Keep signatures in `.github/cla-signatures/`
- Use a protected branch
- Only allow CLA Assistant to write to it

## Testing

1. **Test as a Contributor:**
   - Fork the repository
   - Make a small change
   - Open a pull request
   - Verify you're prompted to sign the CLA

2. **Test as a Maintainer:**
   - Check that PRs without signed CLAs are blocked
   - Verify PR status updates correctly
   - Test that signed CLAs allow PRs to proceed

## Troubleshooting

### CLA Assistant Not Working

1. **Check App Installation:**
   - Go to repository settings → Integrations → Installed GitHub Apps
   - Verify CLA Assistant is installed
   - Check that it has necessary permissions

2. **Check Configuration:**
   - Verify `CLA.md` path is correct
   - Check signature storage location is accessible
   - Review CLA Assistant logs in repository settings

3. **Check Permissions:**
   - CLA Assistant needs read/write access to repository
   - Needs permission to update PR status
   - Needs permission to comment on PRs

### GitHub Actions Workflow Not Running

1. **Check Workflow Permissions:**
   - Go to repository settings → Actions → General
   - Ensure "Workflow permissions" allows read/write access

2. **Check Workflow File:**
   - Verify `.github/workflows/cla-check.yml` exists
   - Check YAML syntax is correct
   - Ensure triggers are set correctly

## Best Practices

1. **Keep CLA Updated:**
   - Version your CLA document
   - Notify contributors of significant changes
   - Consider requiring re-signing for major changes

2. **Documentation:**
   - Keep CONTRIBUTING.md up to date
   - Clearly explain the CLA requirement
   - Provide links to CLA document

3. **Automation:**
   - Use CLA Assistant App for best automation
   - Set up branch protection rules requiring CLA check
   - Configure status checks to block merging without CLA

4. **Transparency:**
   - Make CLA document easily accessible
   - Explain why CLA is required
   - Be clear about what contributors are agreeing to

## Branch Protection Rules

Set up branch protection to require CLA check:

1. Go to repository settings → Branches
2. Add rule for `main` (and other protected branches)
3. Under "Require status checks to pass before merging":
   - Enable "Require branches to be up to date"
   - Add `cla/signature` check
   - Save changes

This ensures PRs cannot be merged without a signed CLA.

## Support

For issues with CLA Assistant:
- CLA Assistant Documentation: https://github.com/cla-assistant/github-action
- GitHub App: https://github.com/apps/cla-assistant
- Support: Check repository issues or discussions


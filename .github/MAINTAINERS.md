# Maintainer Guide

This guide is for ShipOS maintainers and project owners.

## CLA Management

### Verifying CLA Signatures

#### Using CLA Assistant App (Recommended)

1. **View Signatures:**
   - Go to repository settings
   - Navigate to "CLA Assistant"
   - View the list of signed contributors
   - Check signature details (date, IP, version)

2. **Manual Verification:**
   - If a contributor claims to have signed but the check fails:
     - Check CLA Assistant logs
     - Verify the signature in the storage location
     - Manually approve if signature is valid

#### Manual Verification Process

If not using CLA Assistant, verify signatures manually:

1. **Check PR Comments:**
   - Look for comments like: "I have read the CLA.md and agree to its terms"
   - Verify the comment is from the PR author
   - Check that the comment explicitly mentions signing the CLA

2. **Document Signature:**
   - Record the signature in your signature storage
   - Note the date, PR number, and contributor username
   - Update the PR status manually

3. **Update PR Status:**
   ```bash
   # Use GitHub CLI or web interface to update status
   gh pr status --repo owner/ShipOS
   ```

### Handling CLA Issues

#### Contributor Hasn't Signed

1. **First Reminder:**
   - The automated bot should comment automatically
   - Wait 24-48 hours for response

2. **Follow-up:**
   - If no response, comment again with a friendly reminder
   - Provide direct link to CLA.md
   - Offer to answer questions

3. **Final Notice:**
   - If still no response after a week, close the PR with explanation
   - Keep the PR open for when they're ready to sign

#### Contributor Refuses to Sign

- **Politely decline the contribution**
- Explain that CLA is required for all contributions
- Offer to discuss concerns if they have any
- Keep the door open for future contributions

#### CLA Signature Disputes

- Review the signature carefully
- Check if it was signed before or after the contribution
- Verify the signature matches the contributor
- If in doubt, ask the contributor to re-sign

### Updating the CLA

If you need to update the CLA:

1. **Version the CLA:**
   - Update version number in CLA.md
   - Document what changed
   - Consider requiring re-signing for major changes

2. **Notify Contributors:**
   - Announce the change in discussions/announcements
   - Explain why the change is needed
   - Provide a reasonable transition period

3. **Update CLA Assistant:**
   - If using CLA Assistant, update the configuration
   - Set `requireReSigningOnUpdate: true` if needed
   - Test the update process

## Branch Protection

### Setting Up Branch Protection

1. **Go to Repository Settings:**
   - Settings → Branches
   - Add branch protection rule

2. **Configure Rules:**
   - **Branch name pattern:** `main` (and other protected branches)
   - **Require pull request reviews:** Enable
   - **Require status checks:** Enable
     - Add `cla/signature` check
     - Add `build-and-test` check (from CI)
   - **Require branches to be up to date:** Enable
   - **Do not allow bypassing:** Recommended for security

3. **Save Changes**

This ensures:
- PRs cannot be merged without signed CLA
- PRs must pass all CI checks
- Code review is required

## Review Process

### Before Reviewing

1. **Check CLA Status:**
   - Verify CLA is signed (automated check should show this)
   - If not signed, request signature before reviewing

2. **Check CI Status:**
   - Ensure all CI checks pass
   - Review any warnings or errors

3. **Check Code Style:**
   - Verify code is formatted
   - Check follows Allman style
   - Review include organization

### During Review

1. **Code Quality:**
   - Functionality correctness
   - Code style and formatting
   - Documentation and comments
   - Error handling
   - Performance considerations

2. **Architecture:**
   - Fits with existing design
   - Doesn't break existing functionality
   - Follows project patterns

3. **Testing:**
   - Code has been tested
   - Builds successfully
   - No new warnings

### After Review

1. **Approve:**
   - If everything looks good, approve the PR
   - Add any final comments
   - Merge when ready

2. **Request Changes:**
   - Be specific about what needs to change
   - Provide examples if helpful
   - Be constructive and respectful

3. **Merge:**
   - Use "Squash and merge" for cleaner history
   - Write a clear merge commit message
   - Verify CLA is signed before merging

## Handling Contributions Without CLA

**Do not merge PRs without a signed CLA**, even if:
- The code is excellent
- The contributor is well-known
- It's a small change
- The contributor promises to sign later

**Why?**
- Legal protection for the project
- Ensures clear ownership
- Prevents future disputes
- Maintains project integrity

## Signature Storage

### Recommended Structure

```
cla-signatures/
├── README.md
├── signatures/
│   ├── 2024/
│   │   ├── 01/
│   │   │   ├── username1.json
│   │   │   └── username2.json
│   │   └── 02/
│   └── ...
└── index.json  # Quick lookup index
```

### Signature File Format

```json
{
  "username": "contributor-username",
  "email": "contributor@example.com",
  "githubId": 12345678,
  "signedAt": "2024-01-15T10:30:00Z",
  "claVersion": "1.0",
  "prNumber": 42,
  "ipAddress": "xxx.xxx.xxx.xxx",
  "userAgent": "Mozilla/5.0...",
  "verified": true
}
```

## Automation

### GitHub Actions

The repository includes:
- `.github/workflows/cla-check.yml` - Checks CLA status on PRs
- `.github/workflows/ci.yml` - Builds and tests code

### CLA Assistant App

If using CLA Assistant:
- Automatic signature verification
- Automatic PR status updates
- Signature storage management
- Re-signing on CLA updates

## Best Practices

1. **Be Consistent:**
   - Always require CLA before merging
   - Don't make exceptions
   - Treat all contributors equally

2. **Be Clear:**
   - Explain why CLA is required
   - Make the process easy
   - Answer questions promptly

3. **Be Respectful:**
   - Thank contributors for their work
   - Be patient with the process
   - Value all contributions

4. **Document Everything:**
   - Keep CLA up to date
   - Document any changes
   - Maintain clear records

## Troubleshooting

### CLA Check Not Running

1. Check workflow file exists
2. Verify workflow permissions
3. Check GitHub Actions logs
4. Ensure triggers are correct

### CLA Assistant Not Working

1. Verify app is installed
2. Check app permissions
3. Review app configuration
4. Check app logs in settings

### Signature Verification Issues

1. Check signature storage location
2. Verify file format is correct
3. Check access permissions
4. Review signature data

---

For questions or issues, refer to [CLA_SETUP.md](CLA_SETUP.md) or open an issue.


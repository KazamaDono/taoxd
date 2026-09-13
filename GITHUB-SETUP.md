# Connecting this repo to GitHub over SSH (Windows)

You already have an Ed25519 key at `~/.ssh/id_ed25519` (`SHA256:Qbx/UyKhAzYT2AMbd8q5LjDXzl4PO3cratJdYT/pQVk`), but it is **not** yet on your GitHub account — a test connection returns `Permission denied (publickey)`. Pick **one** of the two paths below.

## Path A — reuse your existing key (fastest)

Run in **Git Bash**:

```bash
# 1. Print your public key and copy the whole line
cat ~/.ssh/id_ed25519.pub

# 2. Add it to GitHub: https://github.com/settings/ssh/new
#    Title: "drblack laptop"   Key type: Authentication Key   Paste the line.

# 3. Test it
ssh -T git@github.com
#    Expect: "Hi <username>! You've successfully authenticated..."
```

## Path B — create a fresh, dedicated key (recommended for a clean setup)

This makes a new key just for GitHub and leaves your existing key untouched.

```bash
# 1. Generate the key (Ed25519). Press Enter to accept the path; set a passphrase if you like.
ssh-keygen -t ed25519 -C "professorblackc6@protonmail.com" -f ~/.ssh/id_ed25519_github

# 2. Start the agent and add the key (Git Bash)
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519_github

# 3. Tell SSH to use this key for github.com
printf 'Host github.com\n  HostName github.com\n  User git\n  IdentityFile ~/.ssh/id_ed25519_github\n  IdentitiesOnly yes\n' >> ~/.ssh/config

# 4. Print the PUBLIC key and copy it
cat ~/.ssh/id_ed25519_github.pub

# 5. Add it at https://github.com/settings/ssh/new  (Authentication Key), then test:
ssh -T git@github.com
```

> **PowerShell note.** The same `ssh-keygen`/`ssh-add`/`ssh -T` commands work in PowerShell (Windows ships OpenSSH). To run the agent as a service once: `Get-Service ssh-agent | Set-Service -StartupType Manual; Start-Service ssh-agent`. To copy the key: `Get-Content ~/.ssh/id_ed25519_github.pub | Set-Clipboard`.

## Create the repository and push

You have two ways to create the remote repo.

**With the GitHub CLI** (if you install it — `winget install GitHub.cli`, then `gh auth login`):

```bash
cd "C:/Users/User/Desktop/diary/exploit-dev-book/code"
gh repo create taoxd --public --source . --remote origin --description "Companion code for the book The Art of Exploit Development" --push
```

**Without the CLI** — create an **empty** repo named `taoxd` at <https://github.com/new> (do **not** add a README/license/.gitignore), then:

```bash
cd "C:/Users/User/Desktop/diary/exploit-dev-book/code"
git remote add origin git@github.com:KazamaDono/taoxd.git
git branch -M main
git push -u origin main
```

## Before you push — check the commit author

Commits in this repo are currently authored as **`призрак <professorblackc6@protonmail.com>`** (your global git identity). If you want a different name/email on a public repo, set it *before* pushing:

```bash
cd "C:/Users/User/Desktop/diary/exploit-dev-book/code"
git config user.name  "your-public-handle"
git config user.email "your-public-email@example.com"
git commit --amend --reset-author --no-edit    # re-stamp the existing commit
```

After pushing, update the clone URL in `README.md` (it currently says `KazamaDono`).

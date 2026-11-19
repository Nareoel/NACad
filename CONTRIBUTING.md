# Contributing to NACad

These rules apply to humans and AI agents alike. Code style is in [docs/CODE_STYLE.md](docs/CODE_STYLE.md).

## Commit and PR messages

Every commit message and PR description has this shape:

```
<subject: imperative, at most 72 characters, e.g. "Add nacad_geom static library and doctest setup">

## Brief description
- One short sentence per problem / fix / improvement.

## Detailed description
Only when it's really needed: the why, trade-offs, migration notes. No fixed limit, but keep it a
reasonable length.
```

- The subject is the commit's first line. For a PR, the PR title plays that role.
- Leave out `## Detailed description` when the brief description says enough.
- No AI attribution: commits and PRs must not contain `Co-Authored-By: Claude ...` trailers,
  "Generated with Claude Code" lines or any similar note that an AI tool wrote them.

### Merge commit message

When a PR is merged, the merge (or squash) commit message contains only the subject, the
`## Brief description` and a link to the PR, where the detailed description lives:

```
<PR title> (#<number>)

## Brief description
- One short sentence per problem / fix / improvement.

PR: https://github.com/Nareoel/NACad/pull/<number>
```

## Secrets and personal data

- Never commit credentials, tokens, API keys, private keys, `.env` files or personal data (emails,
  passwords, addresses). If something ever needs a secret, read it from an environment variable.
- A **gitleaks pre-commit hook** scans every commit for secrets and blocks the commit if it finds one.

### Set up the hook (once per clone)

1. Install [gitleaks](https://github.com/gitleaks/gitleaks) (a single binary). For example, on Linux x64:
   ```bash
   mkdir -p ~/Downloads/gitleaks && cd ~/Downloads/gitleaks
   curl -LO https://github.com/gitleaks/gitleaks/releases/download/v8.30.1/gitleaks_8.30.1_linux_x64.tar.gz
   curl -LO https://github.com/gitleaks/gitleaks/releases/download/v8.30.1/gitleaks_8.30.1_checksums.txt
   sha256sum --check --ignore-missing gitleaks_8.30.1_checksums.txt
   tar -xzf gitleaks_8.30.1_linux_x64.tar.gz gitleaks && install -m 755 gitleaks ~/.local/bin/gitleaks
   ```
2. Enable the repository's hooks. Git doesn't do this on its own, for security reasons:
   ```bash
   git config core.hooksPath .githooks
   ```

If gitleaks isn't installed, the hook prints a warning and lets the commit through. If it reports a
false positive, add the fingerprint it prints to a `.gitleaksignore` file.

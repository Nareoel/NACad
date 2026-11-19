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

## Tests

**All functionality is covered by unit tests.** A feature or fix is only done when its tests exist and
pass. The build and test commands are in [AGENTS.md](AGENTS.md#tests).

- Framework: **doctest**, registered with **CTest**. Each `TEST_CASE` is its own CTest test.
- Layout: `geom/tests/test_<module>.cpp`, mirroring the library modules. Add every new test file to
  the source list in `geom/CMakeLists.txt`.
- **Self-contained test data.** Build meshes in code (tetrahedron, cube, icosphere, open grid,
  deliberately broken cases) or embed small files as strings. Tests never depend on the gitignored
  `samples/`.
- What to test:
  - exact results on tiny inputs where the answer can be computed by hand;
  - invariants (for example half-edge consistency, Euler characteristic, Laplacian row sums = 0);
  - failure and edge cases: empty input, malformed files, degenerate or non-manifold geometry,
    boundaries;
  - floating-point values with `doctest::Approx` and explicit tolerances;
  - a regression test for every bug fixed.
- **Coverage:** `NACAD_COVERAGE=ON` + the `coverage` target (needs `gcovr`). Target: at least 90% line
  coverage for `nacad_geom`, and any uncovered code is justified.
- **Sanitizers:** run the tests with `NACAD_SANITIZE=ON` (ASan + UBSan) before calling a change done.
- **Viewer:** GL calls aren't unit-tested. Keep logic (camera math, picking, selection, tool state)
  free of GL calls so it can be tested, and check rendering by running the viewer.

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

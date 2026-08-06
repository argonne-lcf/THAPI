# Cutting a THAPI release

Example: `0.0.15 -> 0.0.16`. Work from a clean checkout.

## 1. Bump on devel

`configure.ac` is the only version pin in this repo:

```diff
-AC_INIT([thapi],[0.0.15],[bvideau@anl.gov])
+AC_INIT([thapi],[0.0.16],[bvideau@anl.gov])
```

```sh
git checkout devel && git pull --ff-only
# edit configure.ac
git commit -am "0.16"
git push origin devel
```

## 2. Fast-forward master + tag

```sh
git checkout master && git pull --ff-only
git merge --ff-only devel
git tag -a v0.0.16 -m "0.0.16"
git push origin master v0.0.16
```

Always `--ff-only`. If it refuses, master diverged from devel —
reconcile before continuing.

## 3. Update THAPI-spack

In `~/THAPI-spack/` (from `main` branch), add one line to
`packages/thapi/package.py`:

```diff
     version("develop", branch="devel")
+    version("0.0.16", tag="v0.0.16")
     version("0.0.15", tag="v0.0.15")
```

Then skim `git log v0.0.15..v0.0.16` for build-system changes and fix up
`package.py` to match. Two things to watch:

- Open-ended ranges pick up the new tag on their own.
- Anything scoped to `master` or `develop` was written against the *old*
  master. Cutting a release moves master, so re-read those directives —
  a "narrow this once master follows" note is usually now due.

Add the new version to the `install` matrix in `.github/workflows/ci.yml`
too — it is a hardcoded list, so a release nobody adds is a release
nobody builds:

```diff
-        version: ['thapi@0.0.13', 'thapi@0.0.15', 'thapi@master', ...
+        version: ['thapi@0.0.13', 'thapi@0.0.15', 'thapi@0.0.16', 'thapi@master', ...
```

That matters because a stale directive rarely fails at concretization; it
fails later, in `configure`, on a spec nobody tried. Let CI be the check —
a local `spack install` takes hours.

PR it:

```sh
git checkout -b release/thapi-0.0.16
git commit -am "Add thapi@0.0.16"
git push -u origin release/thapi-0.0.16
gh pr create --title "Add thapi@0.0.16" --body "Tracks v0.0.16 release."
```
